#include "codegen.h"
#include "llvm/IR/Verifier.h"
#include "fmt/core.h"
#include "scheme.h"
#include <unordered_set>

using namespace llvm;
using namespace std;


void CodegenIR::forNumber(const NumberE& n)
{
	value_ = ConstantInt::getSigned(IntegerType::get(ctx_, 64), n.value_ << 3); //see scheme.h value tagging
}

void CodegenIR::forBoolean(const BooleanE& b)
{
	value_ = ConstantInt::getSigned(IntegerType::get(ctx_, 1), b.b_? static_cast<int>(Scheme::Tag::True): static_cast<int>(Scheme::Tag::False)); 
}

void CodegenIR::forVar(const Var& var)
{
	auto it = table_.find(var.v_);
	if(it == table_.end()) 
		throw std::runtime_error("undefined variable " + var.v_);

	value_ = it->second;
}

void CodegenIR::forQuote(const Quote&){}

void CodegenIR::forDefine(const Define& def)
{
	table_.clear(); //assume define only in toplevel(TODO support inner define)
	if(def.body_->type_ == Expr::Type::Lambda) {
		const auto& lambda = static_cast<const Lambda&>(*def.body_);
		//auto it = globalFunc_.find(def.name_.v_);
		//if(it != globalFunc_.end()) {
		//	throw std::runtime_error("redefination of function: " + def.name_.v_);
		//}

		vector<Type*> paramTys{lambda.arity(), schemeValType};
		auto funcTy = FunctionType::get(schemeValType, paramTys, false);
		//it->second = funcTy; //store it

		auto func = Function::Create(funcTy, llvm::GlobalValue::ExternalLinkage, def.name_.v_, &module_);

		auto entryBB = BasicBlock::Create(ctx_, "entry", func);
		builder_.SetInsertPoint(entryBB);

		const auto& args = func->args();
		const auto& params = *lambda.params_;
		int i = 0;
		for(auto it = args.begin(); it != args.end(); ++it, ++i) {
			table_[params[i]] = it;
		}

		lambda.body_->accept(*this);
		builder_.CreateRet(value_);

		verifyFunction(*func, &llvm::errs());

		value_ = func;
	}else {
		throw std::runtime_error("unimpl");
	}
}

void CodegenIR::forIf(const If& ifExp)
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

void CodegenIR::forLet(const Let& let)
{
	for(auto& kv: let.binds_) {
		kv.second->accept(*this);
		table_[kv.first] = value_;
	}
	let.body_->accept(*this);
}

void CodegenIR::forLambda(const Lambda&){}

void CodegenIR::forApply(const Apply& app)
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


void CodegenIR::printIR()
{
	module_.print(llvm::outs(), nullptr);
}

void CodegenIR::initializeGlobals()
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

void CodegenIR::forProg(Program& prog)
{
	LLVMContext ctx;
	Module mod("schemeMain", ctx);

	CodegenIR gen(ctx, mod);
	gen.initializeGlobals();

	for(const auto& def: prog.defs_) {
		def->accept(gen);
	}

	if(prog.body_) {
		vector<Expr::Ptr> arg;
		arg.emplace_back(std::move(prog.body_));
		Define main(Var("main"), 
				make_unique<Lambda>(vector<Var>{}, 
					make_unique<Apply>( make_unique<Var>("display"), std::move(arg))));

		main.accept(gen);
	}

	verifyModule(mod, &llvm::errs());

	gen.printIR();
}

void CodegenIR::checkArity(size_t actual, size_t expect)
{
	if(actual != expect) {
		throw std::invalid_argument(fmt::format("expect {} arg(s), but got {}", expect, actual));
	}
}
