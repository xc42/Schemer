#pragma once

#include "ast.h"

#include "llvm/IR/IRBuilder.h"

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

class CodegenIR : public VisitorE
{
public:
	CodegenIR(llvm::LLVMContext& ctx, llvm::Module& md): ctx_(ctx), module_(md), builder_(ctx)
	{
		schemeValType = llvm::Type::getInt64Ty(ctx_);
	}

	~CodegenIR(){};

	void static forProg(Program& prog);

	void printIR();
private:
    void forNumber(const NumberE&) override;
    void forBoolean(const BooleanE&) override;
    void forVar(const Var&) override;
    void forQuote(const Quote&) override;
    void forDefine(const Define&) override;
    void forIf(const If&) override;
	void forLet(const Let&) override;
    void forLambda(const Lambda&) override;
    void forApply(const Apply&) override;

	llvm::LLVMContext& ctx_;
	llvm::Module& module_;
	llvm::IRBuilder<> builder_;
	llvm::Value* value_; //codegen result

	llvm::Type *schemeValType;

	//std::unique_ptr<SymTable> table_;
	std::map<std::string, llvm::Value*> table_;
};


