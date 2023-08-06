#pragma once

#include "ast.h"
#include "bytecode.h"
#include "environment.h"

class Compiler: VisitorE {
public:
    Compiler(Instr::Ptr cont): _cont(std::move(cont)) {}
    static Instr::Ptr Compile(Expr::Ptr expr);
private:
    virtual void forNumber(const NumberE&) override;
    virtual void forBoolean(const BooleanE&) override;
    virtual void forVar(const Var&) override;
    virtual void forQuote(const Quote&) override;
    virtual void forDefine(const Define&) override;
    virtual void forSetBang(const SetBang&) override;
    virtual void forBegin(const Begin&) override;
    virtual void forIf(const If&) override;
	virtual void forLet(const Let&) override;
	virtual void forLetRec(const LetRec&) override;
    virtual void forLambda(const Lambda&) override;
    virtual void forApply(const Apply&) override;

    using EnvironmentPtr = Environment<int>::Ptr;
    
    Instr::Ptr compile(const Expr& expr, EnvironmentPtr& env, Instr::Ptr cont);

    Instr::Ptr              _code;
    Instr::Ptr              _cont;
    EnvironmentPtr          _env;
};

