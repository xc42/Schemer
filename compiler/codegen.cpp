#include "codegen.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;
using namespace std;

void CodegenIR::forNumber(const NumberE& n)
{
	value_ = ConstantInt::getSigned(IntegerType::get(ctx_, 64), n.value_); //TODO: tagging
}

void CodegenIR::forBoolean(const BooleanE& b)
{
	value_ = ConstantInt::getSigned(IntegerType::get(ctx_, 64), b.b_); //TODO: tagging
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
	if(def.body_->type_ == Expr::Type::Lambda) {
		const auto& lambda = static_cast<const Lambda&>(*def.body_);
		vector<Type*> paramTys{lambda.arity(), schemeValType};
		auto funcTy = FunctionType::get(schemeValType, paramTys, false);

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

		verifyFunction(*func);

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
	auto elsBB = BasicBlock::Create(ctx_, "els");
	auto contBB = BasicBlock::Create(ctx_, "ifCont");
	builder_.CreateCondBr(condV, thnBB, elsBB);

	builder_.SetInsertPoint(thnBB);
	ifExp.thn_->accept(*this);
	auto thnV = value_;
	builder_.CreateBr(contBB);
	thnBB = builder_.GetInsertBlock(); //update the current block for phi block

	builder_.SetInsertPoint(elsBB);
	ifExp.els_->accept(*this);
	auto elsV = value_;
	builder_.CreateBr(contBB);
	elsBB = builder_.GetInsertBlock();

	curFunc->getBasicBlockList().push_back(contBB);
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
void CodegenIR::forApply(const Apply&){}


void CodegenIR::printIR()
{
	module_.print(llvm::outs(), nullptr);
}

void CodegenIR::forProg(Program& prog)
{
	Define main(Var("main"), make_unique<Lambda>(vector<Var>{}, std::move(prog.body_)));

	LLVMContext ctx;
	Module mod("schemeMain", ctx);

	CodegenIR gen(ctx, mod);
	main.accept(gen);

	verifyModule(mod);

	gen.printIR();
}
