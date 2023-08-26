#include "bccompiler.h"

using namespace std;

Instr::Ptr ByteCodeCompiler::Compile(Expr& expr) {
    ByteCodeCompiler compiler(Instr::New<Halt>());
    expr.accept(compiler);
    return compiler._code;
}

Instr::Ptr ByteCodeCompiler::compile(const Expr &expr, EnvironmentPtr& env, Instr::Ptr cont) {
    auto oldEnv     = _env;
    auto oldCont    = _cont;

    _env    = env;
    _cont   = cont;
    expr.accept(*this);
    _env    = oldEnv;
    _cont   = oldCont;

    return _code;
}

void ByteCodeCompiler::forNumber(const NumberE& num) {
    _code = Instr::New<Imm>(num.value_, _cont); 
}

void ByteCodeCompiler::forBoolean(const BooleanE& b) {
    _code = Instr::New<Imm>(b.b_, _cont);
}

void ByteCodeCompiler::forVar(const Var& var) {
    _code = Instr::New<MemRef>(_env->find(var.v_), _cont);
}

void ByteCodeCompiler::forQuote(const Quote&) {
    // TODO
}

void ByteCodeCompiler::forDefine(const Define&) {
    // TODO
}

void ByteCodeCompiler::forSetBang(const SetBang&) {
}

void ByteCodeCompiler::forBegin(const Begin&) {}

void ByteCodeCompiler::forIf(const If& if_) {
    auto thnc = compile(*if_.thn_, _env, _cont);
    auto elsc = compile(*if_.els_, _env, _cont);
    _code     = compile(*if_.pred_, _env, Instr::New<Branch>(std::move(thnc), std::move(elsc)));
}

void ByteCodeCompiler::forLet(const Let& let) {
    // TODO
}

void ByteCodeCompiler::forLetRec(const LetRec&) {}

void ByteCodeCompiler::forLambda(const Lambda& lam) {
    auto envEx = _env->extend(_env);
    int loc = 0;
    for (const auto& v: *lam.params_) {
        envEx->bind(v.v_, loc++);
    }
    auto bodyc = compile(*lam.body_, envEx, Instr::New<Ret>(lam.params_->size(), _cont));
    _code      = Instr::New<Closure>(bodyc, _cont);
}

void ByteCodeCompiler::forApply(const Apply& app) {
    // eval args from left to right, then eval operator at last 
    auto compilePrim = [&](const auto& expr) -> Instr::Ptr {
        if (expr->getType() != Expr::Type::Var) return nullptr;
        const auto& var = static_cast<const Var&>(*expr);
        auto cont       = Instr::New<Pop>(2, _cont); // the primitives are binary ops
        const auto& op  = var.v_;
        return op == "+"?  Instr::New<Prim>(Instr::Op::ADD, cont):
               op == "-"?  Instr::New<Prim>(Instr::Op::SUB, cont):
               op == "*"?  Instr::New<Prim>(Instr::Op::MUL, cont):
               op == "*"?  Instr::New<Prim>(Instr::Op::DIV, cont):
               op == "%"?  Instr::New<Prim>(Instr::Op::MOD, cont):
               op == "<"?  Instr::New<Prim>(Instr::Op::LT,  cont):
               op == "<="? Instr::New<Prim>(Instr::Op::LE,  cont):
               op == "=="? Instr::New<Prim>(Instr::Op::EQ,  cont):
               op == ">"?  Instr::New<Prim>(Instr::Op::GT,  cont):
               op == ">="? Instr::New<Prim>(Instr::Op::GE,  cont):
               op == "!="? Instr::New<Prim>(Instr::Op::NEQ, cont): nullptr;
    };

    auto primInstr  = compilePrim(app.operator_);
    auto nxt        = primInstr? primInstr: compile(*app.operator_, _env, Instr::New<Jmp>());
    for (auto rit = app.operands_.rbegin(); rit != app.operands_.rend(); ++rit) {
        nxt = compile(**rit, _env, Instr::New<Push>(std::move(nxt)));
    }

    if (!primInstr) _code = Instr::New<Frame>(std::move(nxt));
}

