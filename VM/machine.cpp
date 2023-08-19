#include "machine.h"


void VirtualMachine::forHalt(const Halt& instr) {
    // halt
}

void VirtualMachine::forImm(const Imm& instr) {
    _acc = instr.getVal();
}

void VirtualMachine::forPrim(const Prim& instr) {
    const auto& var1 = **(_stack.rbegin() + 1), &var2 = *_stack.back();
    if (var1.getType() != Value::Type::Number || var2.getType() != Value::Type::Number) {
        throw std::invalid_argument("expect numbers");
    }

    const auto num1 = static_cast<const Number&>(var1).value_;
    const auto num2 = static_cast<const Number&>(var2).value_;
    switch(instr.getOpCode()) {
        case Instr::Op::ADD: {
            _acc = std::make_shared<Number>(num1 + num2); break;
        }
        case Instr::Op::SUB: {
            _acc = std::make_shared<Number>(num1 - num2); break;
        }
        case Instr::Op::MUL: {
            _acc = std::make_shared<Number>(num1 * num2); break;
        }
        case Instr::Op::DIV: {
            _acc = std::make_shared<Number>(num1 / num2); break;
        }
        case Instr::Op::MOD: {
            _acc = std::make_shared<Number>(num1 % num2); break;
        }
        case Instr::Op::LT: {
            _acc = std::make_shared<Boolean>(num1 < num2); break;
        }
        case Instr::Op::LE: {
            _acc = std::make_shared<Boolean>(num1 <= num2); break;
        }
        case Instr::Op::EQ: {
            _acc = std::make_shared<Boolean>(num1 == num2); break;
        }
        case Instr::Op::GT: {
            _acc = std::make_shared<Boolean>(num1 > num2); break;
        }
        case Instr::Op::GE: {
            _acc = std::make_shared<Boolean>(num1 >= num2); break;
        }
        case Instr::Op::NEQ: {
            _acc = std::make_shared<Boolean>(num1 != num2); break;
        }
        default:{
            throw std::runtime_error(fmt::format("internal error, unexpected primitive operator {}",  static_cast<int>(instr.getOpCode())));
        }
    }
}

void VirtualMachine::forMemRef(const MemRef& instr) {

}

void VirtualMachine::forMemSet(const MemSet& instr) {

}
void VirtualMachine::forBranch(const Branch& instr) {}
void VirtualMachine::forPush(const Push& instr) {}
void VirtualMachine::forClosure(const Closure& instr) {}
void VirtualMachine::forFrame(const Frame& instr) {}
void VirtualMachine::forJmp(const Jmp& instr) {}
void VirtualMachine::forRet(const Ret& instr) {}
