#include "codegen.h"
#include "llvm/IR/Verifier.h"
#include "fmt/core.h"
#include "value.h"
#include "scheme.h"
#include "runtime.h"
#include <sstream>
#include <unordered_set>

using namespace llvm;
using namespace std;


void ExprCodeGen::forNumber(const NumberE& n)
{
	value_ = getSchemeInt(n.value_);
}

void ExprCodeGen::forBoolean(const BooleanE& b)
{
	value_ = ConstantInt::getSigned(IntegerType::get(ctx_, 1), Scheme::toBoolReps(b.b_));
}

void ExprCodeGen::forVar(const Var& var)
{
	auto it = table_.find(var.v_);
	if(it == table_.end()) {
		auto pGlob = module_.getGlobalVariable(var.v_);
		if(pGlob) { value_ = pGlob; }
		else { throw std::runtime_error("undefined variable " + var.v_); }
	}else {
		if(curCtx_.isAssigned(var.v_)) {
			value_ = builder_.CreateLoad(it->second, fmt::format("deref_{}", var.v_));
		} else {
			value_ = it->second;
		}
	}
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
	setBang.e_->accept(*this);
	builder_.CreateStore(value_, table_.at(setBang.v_.v_));
	value_ = ConstantInt::getSigned(IntegerType::get(ctx_, 64), static_cast<int64_t>(Scheme::Tag::Void));
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

		if(curCtx_.isAssigned(kv.first.v_)) {
			auto varAddr = builder_.CreateAlloca(schemeValType, nullptr, fmt::format("alloc_{}", kv.first.v_));
			builder_.CreateStore(value_, varAddr);
			table_[kv.first] = varAddr;
		}else {
			table_[kv.first] = value_;
		}
	}
	let.body_->accept(*this);
}

void ExprCodeGen::forLambda(const Lambda& lam)
{
	FreeVarScanner fvScanner;
	lam.accept(fvScanner);
	const auto& fvs = fvScanner.getFVs();
	
	//codegen for the lifted lambda function
	string liftFnName = FrontEndPass::gensym(fmt::format("{}_lambda", builder_.GetInsertBlock()->getParent()->getName()));
	vector<Type*> paramTys{lam.arity()+1, schemeValType}; //closure, param1, param2 ... param_n
	auto fnType = FunctionType::get(schemeValType, paramTys, false);
	auto lambdaFn = Function::Create(fnType, llvm::GlobalValue::InternalLinkage, liftFnName, &module_);

	const auto& params = *lam.params_;
	SymTable lamTable;
	int i = 0;
	for(auto it = lambdaFn->args().begin()+1; it != lambdaFn->args().end(); ++it) {
		lamTable[params[i++]] = it;
	}
	auto closArg = lambdaFn->args().begin();

	auto entryBB = BasicBlock::Create(ctx_, "entry", lambdaFn);
	IRBuilder<> lambdaBuilder(ctx_);
	lambdaBuilder.SetInsertPoint(entryBB);

	if(!fvs.empty()) {
		auto closAddr = lambdaBuilder.CreateAnd(closArg, ~static_cast<uint64_t>(Scheme::Mask::Closure));
		auto closPtr = lambdaBuilder.CreateIntToPtr(closAddr, closureType_->getPointerTo());
		auto fvsPtr = lambdaBuilder.CreateLoad(lambdaBuilder.CreateStructGEP(closureType_, closPtr, 2), "fvPtr");
		int i = 0;
		for(const auto& fv: fvs) { //TODO fix free variable
			auto fvI = lambdaBuilder.CreateGEP(fvsPtr, llvmInt64(i), fmt::format("fv_{}", i));
			lamTable[fv] = lambdaBuilder.CreateLoad(fvI);
			++i;
		}
	}
	ExprCodeGen genforLambda(lambdaBuilder, module_, curCtx_, lamTable);
	lam.body_->accept(genforLambda);
	lambdaBuilder.CreateRet(genforLambda.getValue());

	//codgen of closure
	auto allocCloFunc = module_.getFunction("allocateClosure");
	vector<Value*> args;
	args.emplace_back( builder_.CreateBitCast(lambdaFn, Type::getInt8PtrTy(ctx_), "lambdaPtr"));
	args.emplace_back(ConstantInt::getSigned(IntegerType::get(ctx_, 32), lam.arity()));
	args.emplace_back(ConstantInt::getSigned(IntegerType::get(ctx_, 32), fvs.size()));
	for(const auto& fv: fvs) {
		args.emplace_back(table_.at(fv));
	}
	value_ = builder_.CreateCall(allocCloFunc, args, "clos");
}

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
		return;\
	}
#define GenCmp(CMP) \
	{\
		checkArity(args.size(), 2);\
		value_ = builder_.CreateCmp(CMP, args[0], args[1]);\
		return;\
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
			return;
		}
		else if(op == "/") {
			checkArity(args.size(), 2);
			auto div = builder_.CreateSDiv(args[0], args[1], "divTmp");
			value_ = builder_.CreateShl(div, 3, "divAns");
			return;
		}
		else if(op == ">") 			GenCmp(CmpInst::Predicate::ICMP_SGT)
		else if(op == ">=") 		GenCmp(CmpInst::Predicate::ICMP_SGE)
		else if(op == "<") 			GenCmp(CmpInst::Predicate::ICMP_SLT)
		else if(op == "<=") 		GenCmp(CmpInst::Predicate::ICMP_SLE)
		else if(op == "=")			GenCmp(CmpInst::Predicate::ICMP_EQ)
		else {
			auto func = module_.getFunction(ProgramCodeGen::simpleMangle(op));
			if(func) {
				checkArity(args.size(), func->arg_size());
				value_ = builder_.CreateCall(func, args);
				return;
			}
		}
	}

	//not a var
	app.operator_->accept(*this);
	//assert rator is closure value
	auto rator = value_;
	auto ratorAddr = builder_.CreateAnd(rator, ~static_cast<uint64_t>(Scheme::Mask::Closure));
	auto closPtr = builder_.CreateIntToPtr(ratorAddr, closureType_->getPointerTo(), "rator");
	auto code = builder_.CreateStructGEP(closureType_, closPtr, 1);
	auto codeAddr = builder_.CreateLoad(code, "codeAddr");
	auto funcType = FunctionType::get(schemeValType, {schemeValType}, true);
	auto funcPtr = builder_.CreateBitCast(codeAddr, funcType->getPointerTo(), "codeFunc");

	args.insert(args.begin(), rator);
	value_ = builder_.CreateCall(funcType, funcPtr, args);
}


void ProgramCodeGen::initializeGlobals()
{
	for(const auto& kv: Runtime::builtinFunc) {
		vector<Type*> paramTys;
		paramTys.resize(kv.second, schemeValType);
		auto funcTy = FunctionType::get(schemeValType, paramTys, false);
		//cout << simpleMangle(kv.first) << endl;
		Function::Create(funcTy, llvm::GlobalObject::ExternalLinkage, simpleMangle(kv.first), &module_);
	}

	//struct Closure (see in scheme.h)
	vector<Type*> elemts { Type::getInt32Ty(ctx_), Type::getInt8PtrTy(ctx_), Type::getInt64Ty(ctx_)->getPointerTo()};
	StructType::create(ctx_, elemts, "Closure");
	//allocateClosure
	auto llvmcharPtrTy = Type::getInt8PtrTy(ctx_);
	auto llvmInt32Ty = Type::getInt32Ty(ctx_);
	auto fixParamTys = vector<Type*>{llvmcharPtrTy, llvmInt32Ty, llvmInt32Ty}; //arity, fvs
	auto fnType = FunctionType::get(schemeValType, fixParamTys, true);
	Function::Create(fnType, llvm::GlobalObject::ExternalLinkage, "allocateClosure", &module_);

}


std::string ProgramCodeGen::simpleMangle(const std::string& s)
{
	ostringstream oss;
	for(char c: s) {
		if( std::isdigit(c) || std::isalpha(c) || c == '_' ) oss << c;
		else oss << '_' << static_cast<int>(c) << '_';
	}
	return oss.str();
}

void ProgramCodeGen::gen(FrontEndPass::Program& prog)
{
	initializeGlobals();

	for(auto& [def, passCtx]: prog) {
		if(def.body_->type_ == Expr::Type::Lambda) {
			const auto& lambda = static_cast<const Lambda&>(*def.body_);

			vector<Type*> paramTys{lambda.arity(), schemeValType};
			auto funcTy = FunctionType::get(schemeValType, paramTys, false);
			auto func = Function::Create(funcTy, llvm::GlobalValue::ExternalLinkage, simpleMangle(def.name_.v_), &module_);

			auto entryBB = BasicBlock::Create(ctx_, "entry", func);
			builder_.SetInsertPoint(entryBB);

			SymTable table;
			const auto& args = func->args();
			const auto& params = *lambda.params_;
			int i = 0;
			for(auto it = args.begin(); it != args.end(); ++it, ++i) {
				if(passCtx.assignedVars.count(params[i]) == 0) { //如果一个变量会被赋值，则为其分配内存，后续llvm的mem2reg会处理为SSA
					table[params[i]] = it;
				}else {
					auto alloc = builder_.CreateAlloca(schemeValType, nullptr, fmt::format("allca_{}" ,params[i].v_));
					table[params[i]] = alloc;
					builder_.CreateStore(it, alloc);
				}
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
