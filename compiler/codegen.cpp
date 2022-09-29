#include "codegen.h"
#include "llvm/IR/Verifier.h"
#include "fmt/core.h"
#include "value.h"
#include "scheme.h"
#include <unordered_set>

using namespace llvm;
using namespace std;


void ExprCodeGen::forNumber(const NumberE& n)
{
	value_ = ConstantInt::getSigned(IntegerType::get(ctx_, 64), Scheme::toFixnumReps(n.value_)); //see scheme.h value tagging
}

void ExprCodeGen::forBoolean(const BooleanE& b)
{
	value_ = ConstantInt::getSigned(IntegerType::get(ctx_, 1), Scheme::toBoolReps(b.b_));
}

void ExprCodeGen::forVar(const Var& var)
{
	auto it = table_.find(var.v_);
	if(it == table_.end()) 
		throw std::runtime_error("undefined variable " + var.v_);

	value_ = it->second;
}

void ExprCodeGen::forQuote(const Quote& qo)
{
	switch(qo.datum_->type_) {
		case Parser::Datum::Type::Number:
		{
			const auto& n = static_cast<Parser::DatumNum&>(*qo.datum_);
			value_ = ConstantInt::getSigned(IntegerType::get(ctx_, 64), Scheme::toFixnumReps(n.value_));
			break;
		}
		case Parser::Datum::Type::Boolean:
		{
			const auto& b =  static_cast<Parser::DatumBool&>(*qo.datum_);
			value_ = ConstantInt::getSigned(IntegerType::get(ctx_, 1), Scheme::toBoolReps(b.value_));
			break;
		}
		case Parser::Datum::Type::Nil:
		{
			value_ =  ConstantInt::getSigned(IntegerType::get(ctx_, 64), static_cast<Scheme::ValueType>(Scheme::Tag::Nil)); 
			break;
		}
		defualt:
		{
			throw std::runtime_error("complex datum should be placed as global variable(using define) and initialize in main");
		}
	}
}


void ExprCodeGen::forSetBang(const SetBang& setBang)
{

}

void ExprCodeGen::forBegin(const Begin& bgn)
{
	for(const auto& e: bgn.es_) {
		e->accept(*this);
	}
}

void ExprCodeGen::forIf(const If& ifExp)
{
	ifExp.pred_->accept(*this);
	auto condV = value_;

	auto curFunc = builder_.GetInsertBlock()->getParent();
	auto thnBB = BasicBlock::Create(ctx_, "thn", curFunc);
	auto elsBB = BasicBlock::Create(ctx_, "els", curFunc);
	auto contBB = BasicBlock::Create(ctx_, "ifCont", curFunc);
	builder_.CreateCondBr(condV, thnBB, elsBB);

	builder_.SetInsertPoint(thnBB);
	ifExp.thn_->accept(*this);
	auto thnV = value_;
	builder_.CreateBr(contBB);
	thnBB = builder_.GetInsertBlock(); //update the current block for phi block

	//curFunc->getBasicBlockList().push_back(elsBB);
	builder_.SetInsertPoint(elsBB);
	ifExp.els_->accept(*this);
	auto elsV = value_;
	builder_.CreateBr(contBB);
	elsBB = builder_.GetInsertBlock();

	//curFunc->getBasicBlockList().push_back(contBB);
	builder_.SetInsertPoint(contBB);
	auto phiV = builder_.CreatePHI(thnV->getType(), 2, "if-phi");

	phiV->addIncoming(thnV, thnBB);
	phiV->addIncoming(elsV, elsBB);
	
	value_ = phiV;
}

void ExprCodeGen::forLet(const Let& let)
{
	for(auto& kv: let.binds_) {
		kv.second->accept(*this);
		table_[kv.first] = value_;
	}
	let.body_->accept(*this);
}

void ExprCodeGen::forLambda(const Lambda&){}

void ExprCodeGen::forApply(const Apply& app)
{
	vector<llvm::Value*> args;
	for(const auto& a: app.operands_) {
		a->accept(*this);
		args.emplace_back(value_);
	}

#define GenArith(INSTR) \
	{\
		checkArity(args.size(), 2);\
		value_ = builder_.Create##INSTR(args[0], args[1]);\
	}
#define GenCmp(CMP) \
	{\
		checkArity(args.size(), 2);\
		value_ = builder_.CreateCmp(CMP, args[0], args[1]);\
	}
		
	if(app.operator_->type_ == Expr::Type::Var) {
		const string& op = static_cast<const Var&>(*app.operator_).v_;

		if(op == "+")				GenArith(Add)
		else if(op == "-") 			GenArith(Sub)
		else if(op == "*") {
			checkArity(args.size(), 2);
			auto n1 = builder_.CreateAShr(args[0], 3, "ft1");
			auto n2 = builder_.CreateAShr(args[1], 3, "ft2");
			auto mulTmp = builder_.CreateMul(n1, n2, "mulTmp");
			value_ = builder_.CreateShl(mulTmp, 3, "mulAns");
		}
		else if(op == "/") {
			checkArity(args.size(), 2);
			auto div = builder_.CreateSDiv(args[0], args[1], "divTmp");
			value_ = builder_.CreateShl(div, 3, "divAns");
		}
		else if(op == ">") 			GenCmp(CmpInst::Predicate::ICMP_SGT)
		else if(op == ">=") 		GenCmp(CmpInst::Predicate::ICMP_SGE)
		else if(op == "<") 			GenCmp(CmpInst::Predicate::ICMP_SLT)
		else if(op == "<=") 		GenCmp(CmpInst::Predicate::ICMP_SLE)
		else if(op == "=")			GenCmp(CmpInst::Predicate::ICMP_EQ)
		else {
			auto func = module_.getFunction(op);
			if(func) {
				checkArity(args.size(), func->arg_size());
				value_ = builder_.CreateCall(func, args);
			}else {
				throw std::invalid_argument("could not find function: " + op);
			}
		}
	}else {
	}
}


void ProgramCodeGen::initializeGlobals()
{
	static const map<string, int> builtinFunc = { 
		{"display", 1},
		{"cons", 2},
		{"car", 1},
		{"cdr", 1},
		{"make-vector", 2},
		{"vector-ref", 2},
		{"vector-length", 1},
		{"vector-set!", 3}
	};
	
	for(const auto& kv: builtinFunc) {
		vector<Type*> paramTys;
		paramTys.resize(kv.second, schemeValType);
		auto funcTy = FunctionType::get(schemeValType, paramTys, false);
		Function::Create(funcTy, llvm::GlobalObject::ExternalLinkage, kv.first, &module_);
	}
}

void ProgramCodeGen::gen(FrontEndPass::Program& prog)
{
	initializeGlobals();

	for(auto& [def, passCtx]: prog) {
		if(def.body_->type_ == Expr::Type::Lambda) {
			const auto& lambda = static_cast<const Lambda&>(*def.body_);

			vector<Type*> paramTys{lambda.arity(), schemeValType};
			auto funcTy = FunctionType::get(schemeValType, paramTys, false);
			auto func = Function::Create(funcTy, llvm::GlobalValue::ExternalLinkage, def.name_.v_, &module_);

			auto entryBB = BasicBlock::Create(ctx_, "entry", func);
			builder_.SetInsertPoint(entryBB);

			SymTable table;
			const auto& args = func->args();
			const auto& params = *lambda.params_;
			int i = 0;
			for(auto it = args.begin(); it != args.end(); ++it, ++i) {
				table[params[i]] = it;
			}

			ExprCodeGen exprGen(builder_, module_, passCtx, table);
			lambda.body_->accept(exprGen);
			builder_.CreateRet(exprGen.getValue());

			verifyFunction(*func, &llvm::errs());

		}else {
			throw std::runtime_error("unimpl");
		}

	}

	verifyModule(module_, &llvm::errs());

}


void ProgramCodeGen::printIR()
{
	module_.print(llvm::outs(), nullptr);
}

void ExprCodeGen::checkArity(size_t actual, size_t expect)
{
	if(actual != expect) {
		throw std::invalid_argument(fmt::format("expect {} arg(s), but got {}", expect, actual));
	}
}
