#include "bcdumper.h"

using namespace std;

void InstrDumper::dump(Instr& instr) {
    _worklist.push(&instr);
    while (!_worklist.empty()) {
        auto next = _worklist.front();
        _worklist.pop();

        while (next) {
            next->accept(*this);
            next = _next;
        }
        _os << endl;
    }
}

void InstrDumper::forHalt(const Halt& instr) {
    dumpInstrAddr(&instr);
    _os << "halt" << endl;
    _next = nullptr;
}

void InstrDumper::forImm(const Imm& instr) {
    dumpInstrAddr(&instr);
    _os << "imm " ;
    const auto& val = *instr.getVal();
    auto vty        = val.getType();
    if (vty == Value::Type::Number) {
        _os << static_cast<const Number&>(val).value_;
    } else if (vty == Value::Type::Boolean) {
        _os << static_cast<const Boolean&>(val).value_;
    } else {
        _os << "n/a";
    }
    _os << endl;

    _next = instr.getNext().get();
}

void InstrDumper::forPrim(const Prim& instr) {
    dumpInstrAddr(&instr);
    _os << Instr::to_string(instr.getOpCode()) << endl;
    _next = instr.getNext().get();
}

void InstrDumper::forMemRef(const MemRef& instr) {
    dumpInstrAddr(&instr);
    _os << "mref " << instr.getOffSet() << endl;
    _next = instr.getNext().get();
}

void InstrDumper::forMemSet(const MemSet& instr) {
    //dumpInstrAddr(&instr);
    //_os << "mset" << 
}

void InstrDumper::forBranch(const Branch& instr) {
    dumpInstrAddr(&instr);
    _os << "branch " << instr.getTrue().get() << " " << instr.getFalse().get() << endl;
    _next = instr.getTrue().get();
    _worklist.push(instr.getFalse().get());
}

void InstrDumper::forPush(const Push& instr) {
    dumpInstrAddr(&instr);
    _os << "push" << endl;
    _next = instr.getNext().get();
}

void InstrDumper::forPop(const Pop& instr) {
    dumpInstrAddr(&instr);
    _os << "pop";
    if (instr.getNum() > 1) 
        _os << " " << instr.getNum();
    _os << endl;
    _next = instr.getNext().get();
}

void InstrDumper::forClosure(const Closure& instr) {
    dumpInstrAddr(&instr);
    _os << "closure " << instr.getCode().get() << endl;
    _worklist.push(instr.getCode().get());
    _next = instr.getNext().get();
}

void InstrDumper::forFrame(const Frame& instr) {
    dumpInstrAddr(&instr);
    _os << "frame " << instr.getRet().get() <<  endl;
    _worklist.push(instr.getRet().get());
    _next = instr.getNext().get();
}

void InstrDumper::forCall(const Call& instr) {
    dumpInstrAddr(&instr);
    _os << "call" << endl;
    _next = nullptr;
}

void InstrDumper::forRet(const Ret& instr) {
    dumpInstrAddr(&instr);
    _os << "ret " << instr.getPop() << endl;
    _next = nullptr;
}
