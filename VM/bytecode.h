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
        Closure,
        Frame,
        Jmp,
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
    virtual void accept(InstrVisitor&) override;
private:

    Value::Ptr  _value;
    Ptr                 _next;
};

class Prim: public Instr {
public:
    Prim(Op op, Ptr nxt): Instr(op), _next(std::move(nxt)) {}
    virtual ~Prim()=default;

    virtual void accept(InstrVisitor&) override;
private:

    Ptr                 _next;
};

class MemRef: public Instr {
public:
    MemRef(int offset, Ptr nxt): Instr(Op::Ref), _offset(offset), _next(std::move(nxt)) {}
    virtual ~MemRef()=default;

    int getOffSet() const { return _offset; }
    virtual void accept(InstrVisitor&) override;

private:

    int                 _offset;
    Ptr                 _next;
};

class MemSet: public Instr {
public:
    MemSet(int offset, Value::Ptr v, Ptr nxt): Instr(Op::Set), _value(std::move(v)), _next(std::move(nxt)) {}
    virtual ~MemSet()=default;

    virtual void accept(InstrVisitor&) override;
private:

    int                 _offset;
    Value::Ptr          _value;
    Ptr                 _next;
};

class Branch: public Instr {
public:
    Branch(Ptr t, Ptr f): Instr(Op::Branch), _true(std::move(t)), _false(std::move(f)) { }
    virtual ~Branch()=default;

    virtual void accept(InstrVisitor&) override;
private:

    Ptr                 _true;
    Ptr                 _false;
};


class Push: public Instr {
public:
    Push(Ptr nxt): Instr(Op::Push), _next(std::move(nxt)) {}
    virtual ~Push()=default;

    virtual void accept(InstrVisitor&) override;
private:

    Ptr                 _next;
};

class Closure: public Instr {
public:
    Closure(Ptr code, Ptr nxt): Instr(Op::Closure), _code(std::move(code)), _next(std::move(nxt)) {}
    virtual ~Closure()=default;

    virtual void accept(InstrVisitor&) override;
private:

    Ptr                 _code;
    Ptr                 _next;
};

class Frame: public Instr {
public:
    Frame(Ptr nxt): Instr(Op::Frame), _next(std::move(nxt)) {}
    virtual ~Frame()=default;

    virtual void accept(InstrVisitor&) override;
private:
    Ptr                 _next;
};

class Jmp: public Instr {
public:
    Jmp(): Instr(Op::Jmp) {}
    virtual ~Jmp()=default;

    virtual void accept(InstrVisitor&) override;
private:
};

class Ret: public Instr {
public:
    Ret(int n, Ptr nxt): Instr(Op::Ret), _popn(n), _next(std::move(nxt)) {}
    virtual ~Ret()=default;

    virtual void accept(InstrVisitor&) override;
private:

    int                 _popn;
    Ptr                 _next; // return address
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
    virtual void forClosure(const Closure&) = 0;
    virtual void forFrame(const Frame&) = 0;
    virtual void forJmp(const Jmp&) = 0;
    virtual void forRet(const Ret&) = 0;
};

inline void Halt::accept(InstrVisitor& v) { v.forHalt(*this); }
inline void Imm::accept(InstrVisitor& v) { v.forImm(*this); }
inline void Prim::accept(InstrVisitor& v) { v.forPrim(*this); }
inline void MemRef::accept(InstrVisitor& v) { v.forMemRef(*this); }
inline void Branch::accept(InstrVisitor& v) { v.forBranch(*this); }
inline void Push::accept(InstrVisitor& v) { v.forPush(*this); }
inline void Closure::accept(InstrVisitor& v) { v.forClosure(*this); }
inline void Frame::accept(InstrVisitor& v) { v.forFrame(*this); }
inline void Jmp::accept(InstrVisitor& v) { v.forJmp(*this); }
inline void Ret::accept(InstrVisitor& v) { v.forRet(*this); }


