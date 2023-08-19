#pragma once

#include "bytecode.h"


class VirtualMachine: public InstrVisitor {
public:

    static Value::Ptr execute(Instr::Ptr  instr) {
        VirtualMachine vm;
        instr->accept(vm);
        return std::move(vm._acc);
    }

private:
    virtual void forHalt(const Halt&) override;
    virtual void forImm(const Imm&) override;
    virtual void forPrim(const Prim&) override;
    virtual void forMemRef(const MemRef&) override;
    virtual void forMemSet(const MemSet&) override;
    virtual void forBranch(const Branch&) override;
    virtual void forPush(const Push&) override;
    virtual void forClosure(const Closure&) override;
    virtual void forFrame(const Frame&) override;
    virtual void forJmp(const Jmp&) override;
    virtual void forRet(const Ret&) override;

    std::vector<Value::Ptr>             _stack;

    //registers
    int                                 _bp;
    int                                 _sp;
    Instr::Ptr                          _ip;
    Value::Ptr                          _acc; //accumulator
};
