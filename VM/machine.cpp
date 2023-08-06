#include "machine.h"



void VirtualMachine::forHalt(const Halt& instr) {
    // halt
}

void VirtualMachine::forImm(const Imm& instr) {
    _acc = instr.getVal();
}

void VirtualMachine::forPrim(const Prim& instr) {
    switch(instr.getOpCode()) {
        case Instr::Op::ADD: {

        }
        case Instr::Op::SUB: {
        }
        case Instr::Op::MUL: {
        }
        default:{
        }
    }
}

void VirtualMachine::forMemRef(const MemRef& instr) {
}

void VirtualMachine::forBranch(const Branch& instr) {}
void VirtualMachine::forPush(const Push& instr) {}
void VirtualMachine::forClosure(const Closure& instr) {}
void VirtualMachine::forFrame(const Frame& instr) {}
void VirtualMachine::forJmp(const Jmp& instr) {}
void VirtualMachine::forRet(const Ret& instr) {}
