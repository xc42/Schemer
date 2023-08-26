#pragma once

#include "bytecode.h"
#include "value.h"

#include <ostream>
#include <queue>


class InstrDumper: public InstrVisitor {
public:
    InstrDumper(std::ostream& os): _os(os) {}

    void dump(Instr& instr);
protected:
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

private:
    void dumpInstrAddr(const Instr* instr) {
        _os << instr << ":\t";
    }


    std::ostream&                   _os;
    std::queue<Instr*>              _worklist;
    Instr*                          _next;
    //std::unordered_set<Instr*>      _mark;
};
