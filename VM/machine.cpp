#include "machine.h"


void VirtualMachine::forHalt(const Halt& instr) {
    // halt
    _ip = nullptr;
}

void VirtualMachine::forImm(const Imm& instr) {
    _acc    = instr.getVal();
    _ip     = instr.getNext().get();
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
    _ip = instr.getNext().get();
}

void VirtualMachine::forMemRef(const MemRef& instr) {

    // TODO
}

void VirtualMachine::forMemSet(const MemSet& instr) {

    // TODO
}

void VirtualMachine::forBranch(const Branch& instr) {

    // TODO
}


void VirtualMachine::forPush(const Push& instr) {
    _stack.emplace_back(_acc);
    _ip = instr.getNext().get();
}

void VirtualMachine::forPop(const Pop& instr) {
    _stack.resize(_stack.size() - instr.getNum());
    _ip = instr.getNext().get();
}

void VirtualMachine::forClosure(const Closure& instr) {
    // TODO
    _ip = instr.getNext().get();
}

void VirtualMachine::forFrame(const Frame& instr) {
    // TODO
    _ip = instr.getNext().get();
}

void VirtualMachine::forJmp(const Jmp& instr) {
    if (_acc->getType() != Value::Type::Closure) 
        throw std::runtime_error("VM error: expect a closure");

    // TODO
    //auto& clo   = static_cast<VM::Closure&>(*_acc);

}

void VirtualMachine::forRet(const Ret& instr) {
    // TODO
    _ip = instr.getNext().get();

}
