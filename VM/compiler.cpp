#include "compiler.h"

using namespace std;

Instr::Ptr Compiler::Compile(Expr::Ptr expr) {
    Compiler compiler(Instr::New<Halt>());
    expr->accept(compiler);
    return std::move(compiler._code);
}

Instr::Ptr Compiler::compile(const Expr &expr, EnvironmentPtr& env, Instr::Ptr cont) {
    auto oldEnv     = _env;
    auto oldCont    = _cont;

    _env    = env;
    _cont   = cont;
    expr.accept(*this);
    _env    = oldEnv;
    _cont   = oldCont;

    return _code;
}

void Compiler::forNumber(const NumberE& num) {
    _code = Instr::New<Imm>(num.value_, _cont); 
}

void Compiler::forBoolean(const BooleanE& b) {
    _code = Instr::New<Imm>(b.b_, _cont);
}

void Compiler::forVar(const Var& var) {
    _code = Instr::New<MemRef>(_env->find(var.v_), _cont);
}

void Compiler::forQuote(const Quote&) {
    // TODO
}

void Compiler::forDefine(const Define&) {
    // TODO
}

void Compiler::forSetBang(const SetBang&) {
}

void Compiler::forBegin(const Begin&) {}

void Compiler::forIf(const If& if_) {
    auto thnc = compile(*if_.thn_, _env, _cont);
    auto elsc = compile(*if_.els_, _env, _cont);
    _code     = compile(*if_.pred_, _env, Instr::New<Branch>(std::move(thnc), std::move(elsc)));
}

void Compiler::forLet(const Let& let) {
    // TODO
}

void Compiler::forLetRec(const LetRec&) {}

void Compiler::forLambda(const Lambda& lam) {
    auto envEx = _env->extend(_env);
    int loc = 0;
    for (const auto& v: *lam.params_) {
        envEx->bind(v.v_, loc++);
    }
    auto bodyc = compile(*lam.body_, envEx, Instr::New<Ret>(lam.params_->size(), _cont));
    _code      = Instr::New<Closure>(bodyc, _cont);
}

void Compiler::forApply(const Apply& app) {
    // eval args from left to right, then eval operator at last 
    auto compilePrim = [&](const auto& expr) -> Instr::Ptr {
        if (expr->getType() != Expr::Type::Var) return nullptr;
        const auto& var = static_cast<const Var&>(*expr);
        const auto& op  = var.v_;
        return op == "+"?  Instr::New<Prim>(Instr::Op::ADD, _cont):
               op == "-"?  Instr::New<Prim>(Instr::Op::SUB, _cont):
               op == "*"?  Instr::New<Prim>(Instr::Op::MUL, _cont):
               op == "*"?  Instr::New<Prim>(Instr::Op::DIV, _cont):
               op == "%"?  Instr::New<Prim>(Instr::Op::MOD, _cont):
               op == "<"?  Instr::New<Prim>(Instr::Op::LT, _cont):
               op == "<="? Instr::New<Prim>(Instr::Op::LE, _cont):
               op == "=="? Instr::New<Prim>(Instr::Op::EQ, _cont):
               op == ">"?  Instr::New<Prim>(Instr::Op::GT, _cont):
               op == ">="? Instr::New<Prim>(Instr::Op::GE, _cont):
               op == "!="? Instr::New<Prim>(Instr::Op::NEQ, _cont): nullptr;
    };

    auto primInstr  = compilePrim(app.operator_);
    auto nxt        = primInstr? primInstr: compile(*app.operator_, _env, Instr::New<Jmp>());
    for (auto rit = app.operands_.rbegin(); rit != app.operands_.rend(); ++rit) {
        nxt = compile(**rit, _env, Instr::New<Push>(std::move(nxt)));
    }
    _code = Instr::New<Frame>(std::move(nxt));
}

