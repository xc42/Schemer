#pragma once

#include "ast.h"
#include "value.h"
#include "fmt/core.h"
#include <numeric>

namespace Interp {

class Environment {
public:
    using Ptr = std::shared_ptr<Environment>;

    void bind(const std::string& name, const Value::Ptr& val) { bindings_[name] = val; }
    void bind(const Var& v, const Value::Ptr& val) { bindings_[v.v_] = val; }
	Value::Ptr operator()(const Var& s);

    static Ptr extend(const std::vector<std::pair<Var, Value::Ptr>> &binds, const Ptr& old);

    std::map<std::string, Value::Ptr> bindings_;
    Ptr outer_;
};


inline bool checkArityExact(int expect, int actual)
{
    if(expect != actual) {
        throw std::length_error(fmt::format("procedure expect {} args, but got {}", expect, actual));
    }
    return true;
}

inline bool checkArityAtLeast(int least, int actual) 
{
    if(actual < least) {
        throw std::length_error(fmt::format("expect at least {} args, but got {}", least, actual));
    }
    return true;
}

inline bool checkValueType(const Value::Ptr& v, Value::Type t)
{
	if(v->type_ != t) {
		throw std::runtime_error(fmt::format("expect a {1}, but got {0}", typeStr(v->type_), typeStr(t)));
	}
	return true;
}

class Evaluator: public VisitorE {
public:
    Evaluator():
		env_(std::make_shared<Environment>())
	{}

    Evaluator(const Environment::Ptr& env): env_(env){}


    Value::Ptr getResult() { return result_; }

    ~Evaluator()=default;
private:
    void forNumber(const NumberE& num) override { result_ = std::make_unique<Number>(num.value_); }
    void forBoolean(const BooleanE& b) override { result_ = std::make_unique<Boolean>(b.b_); }
    void forVar(const Var& s) override { result_ = (*env_)(s); };
    void forQuote(const Quote& quo) override;
    void forDefine(const Define& def) override;
    void forSetBang(const SetBang& setBang) override {} //TODO
    void forBegin(const Begin& bgn) override{} //TODO
    void forIf(const If&) override;
	void forLet(const Let&) override;
	void forLetRec(const LetRec&) override;
    void forLambda(const Lambda & lambda) override { result_ = std::make_shared<Closure>(std::make_unique<Lambda>(lambda), env_);
	}
    void forApply(const Apply&) override;

    Value::Ptr result_;
    Environment::Ptr env_;
};

class ValuePrinter: public VisitorV
{
public:
	ValuePrinter(std::ostream& os): os_(os) {}

private:
	void forNumber(const Number& n) override {  os_ << n.value_ << '\n'; }
	void forBoolean(const Boolean& b) override {  
		if(b.value_) os_ << "#t" << '\n';
		else os_ << "#f" << '\n';
	}

	void forSymbol(const Symbol& s) override { os_ << *s.ptr_ << '\n'; }

	void forClosure(const Closure&) override { os_ << "#<closure>" << '\n'; }
	void forProcedure(const Procedure&) override { os_ << "#<procedure>" << '\n'; }
	void forCons(const Cons&) override;
	void forVoid(const Void&) override {}
	void forNil(const Nil& n) override { os_ << "()" << '\n'; }

	std::ostream &os_;
};


namespace builtin{

using Args = std::vector<Value::Ptr>;

Value::Ptr cons(const Args&);
Value::Ptr car(const Args&);
Value::Ptr cdr(const Args&);
Value::Ptr nullq(const Args&);
Value::Ptr eqq(const Args&);

template<typename ArithOp>
class Arith {
public:

	Value::Ptr operator()(const Args& args) {
		static ArithOp op;

		checkArityAtLeast(2, args.size());
		checkValueType(args[0], Value::Type::Number);
		auto ans = std::accumulate(args.begin()+1, args.end(), static_cast<Number&>(*args[0]), 
				[this](const Number& acc, const Value::Ptr& n) {
					checkValueType(n, Value::Type::Number);
					return op(acc.value_, static_cast<Number&>(*n).value_);
				});
		return std::make_unique<Number>(ans);
    }
private:
};

template<typename LogicOp>
class Comparator {
public:
    Value::Ptr operator()(const Args& args) {
		LogicOp op;
		checkArityExact(args.size(), 2);
		const auto& l = *args[0], &r = *args[1];
		checkValueType(args[0], Value::Type::Number);
		checkValueType(args[1], Value::Type::Number);
		return std::make_unique<Boolean>(op(static_cast<const Number&>(l).value_, static_cast<const Number&>(r).value_));
    }
private:
};

Environment::Ptr getInitialTopEnv();
} //namespace builtin
} //namespace Interp
