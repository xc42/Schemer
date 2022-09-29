#pragma once

#include "ast.h"

#include "llvm/IR/IRBuilder.h"
#include "front-end-pass.h"

/*
class SymTable  //environment
{
public:
	using Ptr = std::unique_ptr<SymTable>;

	SymTable()=default;
	SymTable(const std::vector<std::pair<std::string, llvm::Value*>>& kvs, Ptr p)
	{
		for(const auto& kv: kvs) table_[kv.first] = kv.second;
		outter_ = std::move(p);
	}

	llvm::Value* find(const std::string& name) {
		auto it = table_.find(name);
		return it != table_.end()?
			it->second:
			(outter_? outter_->find(name): nullptr);
	}

	void bind(const std::string& name, llvm::Value* val) { table_[name] = val; }

	static Ptr extend(const std::vector<std::pair<std::string, llvm::Value*>>& kvs, Ptr p)
	{
		return std::make_unique<SymTable>(kvs, std::move(p));
	}


private:
	std::map<std::string, llvm::Value*> table_;
	Ptr outter_;
};
*/

using SymTable = std::map<std::string, llvm::Value*>;

class ExprCodeGen : public VisitorE
{
public:

	ExprCodeGen(llvm::IRBuilder<>& bder, llvm::Module& md, FrontEndPass::PassContext& ctx, SymTable& tb): 
		builder_(bder), module_(md), ctx_(bder.getContext()), curCtx_(ctx), table_(tb)
	{
		schemeValType = llvm::Type::getInt64Ty(ctx_);
	}

	llvm::Value* getValue() { return value_; }

	~ExprCodeGen(){};
private:

    void forNumber(const NumberE&) override;
    void forBoolean(const BooleanE&) override;
    void forVar(const Var&) override;
    void forQuote(const Quote&) override;
    void forDefine(const Define&) override {}; //TODO
    void forSetBang(const SetBang&) override;
    void forBegin(const Begin&) override;
    void forIf(const If&) override;
	void forLet(const Let&) override;
    void forLambda(const Lambda&) override;
    void forApply(const Apply&) override;

	static void checkArity(size_t actual, size_t expect);


	llvm::Module& module_;
	llvm::IRBuilder<>& builder_;
	llvm::LLVMContext& ctx_;

	llvm::Value* value_; //codegen result

	FrontEndPass::PassContext curCtx_; //current function pass context info


	//std::unique_ptr<SymTable> table_;
	std::map<std::string, llvm::Value*> table_;
	std::map<std::string, llvm::FunctionType*> globalFunc_;
	llvm::Type *schemeValType;
};

class ProgramCodeGen
{
public:
	ProgramCodeGen(): module_("schemeMain", ctx_), builder_(ctx_) 
	{
		schemeValType = llvm::Type::getInt64Ty(ctx_);
	}

	void gen(FrontEndPass::Program& prog);
	void printIR();
private:
	void initializeGlobals();

	llvm::LLVMContext ctx_;
	llvm::Module module_;
	llvm::IRBuilder<> builder_;
	llvm::Type *schemeValType;

};
