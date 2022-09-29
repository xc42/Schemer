#pragma once

#include "ast.h"
#include <functional>
#include <unordered_set>

namespace Interp
{

class VisitorV;
// Value = Number| Symbol| Appliable| Cons| Boolean
// Appliable = LambdaV| Procedure
struct Value {
    using Ptr = std::shared_ptr<Value>;
    enum class Type {Number,  Boolean, Symbol, Closure, Procedure, Cons, Nil, Void} type_;

    Value(Type t):type_(t){}
    virtual void accept(VisitorV& visitor) const=0;
    virtual ~Value()=default;
};


struct Number: public Value {
    using Type = long long;
    Number(Type v):Value(Value::Type::Number), value_(v){};
    void accept(VisitorV &v) const override;
    ~Number()=default;

    Type value_;
};

struct Boolean: public Value {
    Boolean(bool b): Value(Value::Type::Boolean), value_(b) {}
    void accept(VisitorV &v) const override;
    operator bool() { return value_; }
    ~Boolean()=default;

    bool value_;
};

struct Symbol: public Value {
    Symbol(const std::string& s):Value(Value::Type::Symbol), ptr_(intern(s)){}

    void accept(VisitorV &v) const override;
    bool operator<(const Symbol& s) const { return ptr_ < s.ptr_; }
    ~Symbol()=default;

	static const std::string* intern(const std::string& s) {
		static std::unordered_set<std::string> string_intern;
		auto pos = string_intern.insert(s).first;
		return &*pos;
	}

    const std::string *ptr_; //do not need free, handled elsewhere
};

class Environment;
struct Closure: public Value {
	Closure(std::unique_ptr<Lambda> lam, const std::shared_ptr<Environment> &env): 
		Value(Value::Type::Closure),
		lambda_(std::move(lam)), env_(env){}


    void accept(VisitorV &v) const override;

	int arity() { return lambda_->arity(); }

	std::unique_ptr<Lambda> lambda_;
	std::shared_ptr<Environment> env_;
};


struct Procedure: public Value{
public:
    using Func = std::function<Value::Ptr(const std::vector<Value::Ptr>&)>;
    Procedure(const Func& f): Value(Value::Type::Procedure), func_(f){}

    void accept(VisitorV &v) const override;

	Value::Ptr apply(std::vector<Value::Ptr>& args);
    //Value::Ptr apply(const Appliable::Arg& args) const override { return func(args); }
    ~Procedure()=default;


    Func func_;
};

struct Cons: public Value {
    Cons(const Value::Ptr& car=nullptr, const Value::Ptr& cdr=nullptr):
        Value(Value::Type::Cons), car_(car), cdr_(cdr) {}

    void accept(VisitorV &v) const override;
    ~Cons()=default;
    Value::Ptr car_, cdr_;
};

class Nil: public Value {
public:
	static auto& getInstance() {
		static std::shared_ptr<Nil> nil(new Nil);
		return nil;
	}
    ~Nil()=default;
private:
	Nil():Value(Value::Type::Nil) {}

    void accept(VisitorV &v) const override;
};

struct Void: public Value
{
public:
	static auto getInstance()
	{
		std::shared_ptr<Void> void_{ new Void };
		return void_;
	}
    ~Void()=default;
private:
	Void():Value(Value::Type::Void) {}

    void accept(VisitorV &v) const override;
};

class VisitorV
{
public:
	virtual void forNumber(const Number&)=0;
	virtual void forBoolean(const Boolean&)=0;
	virtual void forSymbol(const Symbol&)=0;
	virtual void forClosure(const Closure&)=0;
	virtual void forProcedure(const Procedure&)=0;
	virtual void forCons(const Cons&)=0;
	virtual void forVoid(const Void&)=0;
	virtual void forNil(const Nil&)=0;

	virtual ~VisitorV()=default;
};

inline void Number::accept(VisitorV& v) const { v.forNumber(*this); }
inline void Boolean::accept(VisitorV& v) const { v.forBoolean(*this); }
inline void Symbol::accept(VisitorV& v) const { v.forSymbol(*this); }
inline void Closure::accept(VisitorV& v) const { v.forClosure(*this); }
inline void Procedure::accept(VisitorV& v) const { v.forProcedure(*this); }
inline void Cons::accept(VisitorV& v) const { v.forCons(*this); }
inline void Void::accept(VisitorV& v) const { v.forVoid(*this); }
inline void Nil::accept(VisitorV& v) const { v.forNil(*this); }

inline const char* typeStr(Value::Type t)
{
	switch(t)
	{
		case Value::Type::Number: return "number";
		case Value::Type::Boolean: return "boolean";
		case Value::Type::Symbol: return "symbol";
		case Value::Type::Cons: return "cons cell";
		case Value::Type::Closure:
		case Value::Type::Procedure: return "procedure";
		case Value::Type::Void: return "void";
		case Value::Type::Nil: return "nil";
		default: return "#unknown#";
	}
}

} //namespace Interp
