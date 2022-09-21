#pragma once

#include "ast.h"

#include "llvm/IR/IRBuilder.h"

class CodegenIR : public VisitorE
{
private:
    virtual void forNumber(const NumberE&)=0;
    virtual void forBoolean(const BooleanE&)=0;
    virtual void forVar(const Var&)=0;
    virtual void forQuote(const Quote&)=0;
    virtual void forDefine(const Define&)=0;
    virtual void forIf(const If&)=0;
	virtual void forLet(const Let&)=0;
    virtual void forLambda(const Lambda&)=0;
    virtual void forApply(const Apply&)=0;
    virtual ~CodegenIR()=default;

	llvm::LLVMContext ctx_;
	llvm::Module module_;
	llvm::IRBuilder<> builder;
};
