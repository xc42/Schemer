#pragma once

#include <vector>
#include <memory>
#include "value.h"

class InstrVisitor;
class Instr {
public: 
    using Ptr = std::shared_ptr<Instr>;
    enum class Op{
        Halt,
        Imm,
        Ref,
        Set,
        Branch,
        Push,
        Pop,
        Closure,
        Frame,
        Call,
        Ret,
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        LT,
        LE,
        EQ,
        GT,
        GE,
        NEQ
    };

    Instr(Op op):_op(op) {}
    virtual ~Instr() {};

    template<class InstrT, class... Ts>
    static Ptr New(Ts&&... args) {
        //return std::make_shared<InstrT>(std::forward<Ts>(args)...);
        return std::shared_ptr<InstrT>(new InstrT(std::forward<Ts>(args)...));
    }

    virtual void accept(InstrVisitor&) = 0;

    auto getOpCode() const { return _op; }

    static std::string_view to_string(Op op) {
        switch (op) {
            case Op::Halt:      return "halt";
            case Op::Imm:       return "imm";
            case Op::Ref:       return "mread";
            case Op::Set:       return "mset";
            case Op::Branch:    return "branch";
            case Op::Push:      return "push";
            case Op::Pop:       return "pop";
            case Op::Closure:   return "closure";
            case Op::Frame:     return "frame";
            case Op::Call:       return "jmp";
            case Op::Ret:       return "ret";
            case Op::ADD:       return "add";
            case Op::SUB:       return "sub";
            case Op::MUL:       return "mul";
            case Op::DIV:       return "div";
            case Op::MOD:       return "mod";
            case Op::LT:        return "lt";
            case Op::LE:        return "le";
            case Op::EQ:        return "eq";
            case Op::GT:        return "gt";
            case Op::GE:        return "ge";
            case Op::NEQ:       return "neq";
            default:            return "unkown-instr";
        }
    }
private:
    Op _op;
};

class Halt: public Instr {
public:
    Halt(): Instr(Op::Halt) {}
    virtual ~Halt()=default;
    virtual void accept(InstrVisitor&) override;
private:
};

class Imm: public Instr {
public:
    Imm(Number::Type v, Ptr nxt):
        Instr(Op::Imm), _value(std::make_shared<Number>(v)), _next(std::move(nxt)) {}
    Imm(bool v, Ptr nxt): 
        Instr(Op::Imm), _value(std::make_shared<Boolean>(v)), _next(std::move(nxt)) {}
    virtual ~Imm()=default;

    const auto getVal() const { return _value; }
    const auto& getNext() const { return _next; }

    virtual void accept(InstrVisitor&) override;
private:

    Value::Ptr  _value;
    Ptr                 _next;
};

class Prim: public Instr {
public:
    Prim(Op op, Ptr nxt): Instr(op), _next(std::move(nxt)) {}
    virtual ~Prim()=default;

    const auto& getNext() const { return _next; }

    virtual void accept(InstrVisitor&) override;
private:

    Ptr                 _next;
};

class MemRef: public Instr {
public:
    MemRef(int offset, Ptr nxt): Instr(Op::Ref), _offset(offset), _next(std::move(nxt)) {}
    virtual ~MemRef()=default;

    const auto& getNext() const { return _next; }
    int getOffSet() const { return _offset; }
    virtual void accept(InstrVisitor&) override;

private:

    int                 _offset;
    Ptr                 _next;
};

class MemSet: public Instr {
public:
    MemSet(int offset, Ptr nxt): Instr(Op::Set), _next(std::move(nxt)) {}
    virtual ~MemSet()=default;

    int getOffSet() const { return _offset; }
    const auto& getNext() const { return _next; }
    virtual void accept(InstrVisitor&) override;
private:

    int                 _offset;
    Ptr                 _next;
};

class Branch: public Instr {
public:
    Branch(Ptr t, Ptr f): Instr(Op::Branch), _true(std::move(t)), _false(std::move(f)) { }
    virtual ~Branch()=default;

    const auto& getTrue() const { return _true; }
    const auto& getFalse() const { return _false; }

    virtual void accept(InstrVisitor&) override;
private:

    Ptr                 _true;
    Ptr                 _false;
};


class Push: public Instr {
public:
    Push(Ptr nxt): Instr(Op::Push), _next(std::move(nxt)) {}
    virtual ~Push()=default;

    const auto& getNext() const { return _next; }
    virtual void accept(InstrVisitor&) override;
private:

    Ptr                 _next;
};

class Pop: public Instr {
public:
    Pop(Ptr nxt): Instr(Op::Pop), _n(1), _next(std::move(nxt)) {}
    Pop(size_t n, Ptr nxt): Instr(Op::Pop), _n(n), _next(std::move(nxt)) {}
    virtual ~Pop()=default;

    size_t getNum() const { return _n; }

    const auto& getNext() const { return _next; }
    virtual void accept(InstrVisitor&) override;
private:

    size_t              _n;
    Ptr                 _next;
};

class Closure: public Instr {
public:
    Closure(Ptr code, Ptr nxt): Instr(Op::Closure), _code(std::move(code)), _next(std::move(nxt)) {}
    virtual ~Closure()=default;

    const auto& getCode() const { return _code; }
    const auto& getNext() const { return _next; }

    virtual void accept(InstrVisitor&) override;
private:

    Ptr                 _code;
    Ptr                 _next;
};

class Frame: public Instr {
public:
    Frame(Ptr ret, Ptr nxt): Instr(Op::Frame), _return(std::move(ret)), _next(std::move(nxt)) {}
    virtual ~Frame()=default;

    const auto& getNext() const { return _next; }
    const auto& getRet() const  { return _return; }

    virtual void accept(InstrVisitor&) override;
private:
    Ptr                 _next;
    Ptr                 _return;
};

class Call: public Instr {
public:
    Call(): Instr(Op::Call) {}
    virtual ~Call()=default;

    virtual void accept(InstrVisitor&) override;
private:
};

class Ret: public Instr {
public:
    Ret(int n): Instr(Op::Ret), _popn(n) {}
    virtual ~Ret()=default;

    int getPop() const { return _popn; }

    virtual void accept(InstrVisitor&) override;
private:

    int                 _popn;
};

class InstrVisitor {
public:
    virtual void forHalt(const Halt&) = 0;
    virtual void forImm(const Imm&) = 0;
    virtual void forPrim(const Prim&) = 0;
    virtual void forMemRef(const MemRef&) = 0;
    virtual void forMemSet(const MemSet&) = 0;
    virtual void forBranch(const Branch&) = 0;
    virtual void forPush(const Push&) = 0;
    virtual void forPop(const Pop&) = 0;
    virtual void forClosure(const Closure&) = 0;
    virtual void forFrame(const Frame&) = 0;
    virtual void forCall(const Call&) = 0;
    virtual void forRet(const Ret&) = 0;
};

inline void Halt::accept(InstrVisitor& v) { v.forHalt(*this); }
inline void Imm::accept(InstrVisitor& v) { v.forImm(*this); }
inline void Prim::accept(InstrVisitor& v) { v.forPrim(*this); }
inline void MemRef::accept(InstrVisitor& v) { v.forMemRef(*this); }
inline void Branch::accept(InstrVisitor& v) { v.forBranch(*this); }
inline void Push::accept(InstrVisitor& v) { v.forPush(*this); }
inline void Pop::accept(InstrVisitor& v) { v.forPop(*this); }
inline void Closure::accept(InstrVisitor& v) { v.forClosure(*this); }
inline void Frame::accept(InstrVisitor& v) { v.forFrame(*this); }
inline void Call::accept(InstrVisitor& v) { v.forCall(*this); }
inline void Ret::accept(InstrVisitor& v) { v.forRet(*this); }

