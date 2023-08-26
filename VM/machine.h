#pragma once

#include "bytecode.h"


class VirtualMachine: public InstrVisitor {
public:
    VirtualMachine() :
        _bp(0),
        //_sp(0),
        _ip(nullptr) {
    }

    const Value* getResult() const { return _acc.get(); }

    Value::Ptr execute(Instr&  instr) {
        _ip = &instr;
        while (_ip) {
            _ip->accept(*this);
        }
        return _acc;
    }

private:
    virtual void forHalt(const Halt&) override;
    virtual void forImm(const Imm&) override;
    virtual void forPrim(const Prim&) override;
    virtual void forMemRef(const MemRef&) override;
    virtual void forMemSet(const MemSet&) override;
    virtual void forBranch(const Branch&) override;
    virtual void forPush(const Push&) override;
    virtual void forPop(const Pop&) override;
    virtual void forClosure(const Closure&) override;
    virtual void forFrame(const Frame&) override;
    virtual void forJmp(const Jmp&) override;
    virtual void forRet(const Ret&) override;

    std::vector<Value::Ptr>             _stack; // evalution stack

    //registers
    int                                 _bp; // stack base pointer
    //int                                 _sp; // stack top pointer
    Instr*                              _ip; // instruction pointer
    Value::Ptr                          _acc; //accumulator
};
