#pragma once

#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <optional>

class VisitorE; //forward declaration of base visitor

//Expr = NumberE| SymbolE| Quote| Define| If| Lambda| Application
struct Expr {
    using Ptr = std::unique_ptr<Expr>;

    enum class Type {Number, Symbol, Quote, Define, If, Lambda, Application} type_;

    Expr(Type t):type_(t){}
    virtual void accept(VisitorE& visitor) const = 0;
    virtual ~Expr()=default;
};

struct NumberE: Expr{
    using Type = long long;
    NumberE(Type v): Expr(Expr::Type::Number), value_(v){};
    void accept(VisitorE &v) const override;

    Type value_;
};

struct SymbolE: Expr{
    SymbolE(const std::string* p):Expr(Expr::Type::Symbol), ptr_(p){}
    void accept(VisitorE &v) const override;
    bool operator<(const SymbolE& s) const { return ptr_ < s.ptr_; }
    const std::string *ptr_;    
};

struct Define: Expr {
    Define(const SymbolE& n, Expr::Ptr &&b):
        Expr(Expr::Type::Define), name_(n), body_(std::move(b)){}

    void accept(VisitorE &v) const override;

    SymbolE name_;
    Expr::Ptr body_;
};

struct If: Expr{
    If(Expr::Ptr &&p, Expr::Ptr &&c, Expr::Ptr &&a):
        Expr(Expr::Type::If), pred_(std::move(p)), conseq_(std::move(c)), alter_(std::move(a)){}

    void accept(VisitorE &v) const override;

    Expr::Ptr pred_, conseq_, alter_;
};

struct LambdaE: Expr{
    using ParamsType = std::vector<SymbolE>;
    LambdaE(const ParamsType& params, Expr::Ptr&& b):
        Expr(Expr::Type::Lambda), params_(std::move(params)), body_(std::move(b)){}

    void accept(VisitorE &v) const override;

    ParamsType params_;
    Expr::Ptr body_;
};

struct Application: Expr{
    using Operands = std::vector<Expr::Ptr>;
    Application(Expr::Ptr &&rator, Operands&& rands):
        Expr(Expr::Type::Application), operator_(std::move(rator)), operands_(std::move(rands)){}

    void accept(VisitorE &v) const override;

    Expr::Ptr operator_;
    Operands operands_;
};

class VisitorV;
// Value = Number| Symbol| Appliable| Cons| Boolean
// Appliable = LambdaV| Procedure
struct Value {
    using Ptr = std::shared_ptr<Value>;
    enum class Type {Number, Symbol, Appliable, Cons, Boolean} type_;

    Value(Type t):type_(t){}
    virtual void accept(VisitorV& visitor) const=0;
    virtual ~Value()=default;
};

struct Quote: Expr{
    Quote(const Value::Ptr& v):Expr(Expr::Type::Quote), data_(v){}
    void accept(VisitorE &v) const override;

    Value::Ptr data_;
};

struct NumberV: public Value {
    using Type = long long;
    NumberV(Type v):Value(Value::Type::Number), value_(v){};
    void accept(VisitorV &v) const override;
    ~NumberV()=default;

    Type value_;
};

struct SymbolV: public Value {
    SymbolV(const SymbolE& s):Value(Value::Type::Symbol), ptr_(s.ptr_){}
    void accept(VisitorV &v) const override;
    bool operator<(const SymbolV& s) const { return ptr_ < s.ptr_; }
    ~SymbolV()=default;

    const std::string *ptr_; //do not need free, handled elsewhere
};

struct Appliable: public Value {
    using Arg = std::vector<Value::Ptr>;
    enum class Type { Lambda, BuildIn } func_type_;

    Appliable(Type t):Value(Value::Type::Appliable), func_type_(t){}
    virtual Value::Ptr apply(const Arg&) const =0;
    virtual ~Appliable()=default;
};

class Environment;
struct LambdaV: public Appliable{
    LambdaV(std::unique_ptr<LambdaE>&& e, const std::shared_ptr<Environment>& env):
        Appliable(Appliable::Type::Lambda), expr_(std::move(e)), env_(env) {}

    void accept(VisitorV &v) const override;
    Value::Ptr apply(const Appliable::Arg&) const override;

    std::unique_ptr<LambdaE> expr_;
    std::shared_ptr<Environment> env_;
};

class Procedure: public Appliable {
public:
    using Func = std::function<Value::Ptr(const Appliable::Arg&)>;
    Procedure(const Func& f):Appliable(Appliable::Type::BuildIn), func(f){}

    void accept(VisitorV &v) const override;
    Value::Ptr apply(const Appliable::Arg& args) const override { return func(args); }
    ~Procedure()=default;
private:
    Func func;
};

struct Cons: public Value {
    Cons(const Value::Ptr& car=nullptr, const Value::Ptr& cdr=nullptr):
        Value(Value::Type::Cons), car_(car), cdr_(cdr) {}

    void accept(VisitorV &v) const override;
    ~Cons()=default;
    Value::Ptr car_, cdr_;
};

struct Boolean: public Value {
    Boolean(bool b): Value(Value::Type::Boolean), value_(b) {}
    void accept(VisitorV &v) const override;
    operator bool() { return value_; }
    ~Boolean()=default;

    bool value_;
};

template<typename ValueType>
constexpr const char* type_string() {
    if constexpr (std::is_same_v<ValueType, NumberV>) return "number";
    else if constexpr(std::is_same_v<ValueType, SymbolV>) return "symbol";
    else if constexpr(std::is_same_v<ValueType, Appliable>) return "Appliable";
    else if constexpr(std::is_same_v<ValueType, LambdaV>) return "#<procedure>";
    else if constexpr(std::is_same_v<ValueType, Procedure>) return "#<buildin-procedure>";
    else if constexpr(std::is_same_v<ValueType, Cons>) return "cons-struct";
    else if constexpr(std::is_same_v<ValueType, Boolean>) return "boolean";
    else return "any type";
}

class Environment {
public:
    using Ptr = std::shared_ptr<Environment>;

    void bind(const SymbolE& name, const Value::Ptr& val);
    static Ptr extend(const LambdaE::ParamsType&, const Appliable::Arg&, const Ptr& old);

    std::optional<Value::Ptr> operator()(const SymbolE& s) { 
        auto it = bindings_.find(s);
        return it != bindings_.end()? it->second: (outer_? (*outer_)(s): std::nullopt);
    }

private:
    std::map<SymbolE, Value::Ptr> bindings_;
    Ptr outer_;
};


class VisitorE {
public:
    virtual void forNumber(const NumberE&)=0;
    virtual void forSymbol(const SymbolE&)=0;
    virtual void forQuote(const Quote&)=0;
    virtual void forDefine(const Define&)=0;
    virtual void forIf(const If&)=0;
    virtual void forLambda(const LambdaE&)=0;
    virtual void forApplication(const Application&)=0;
    virtual ~VisitorE()=default;
};

class VisitorV {
public:
    virtual void forNumber(const NumberV&)=0;
    virtual void forSymbol(const SymbolV&)=0;
    virtual void forAppliable(const Appliable&)=0;
    virtual void forCons(const Cons&)=0;
    virtual void forBoolean(const Boolean&)=0;
    virtual ~VisitorV()=default;
};

class Evaluator: public VisitorE {
public:
    Evaluator():env_(std::make_shared<Environment>()){}
    Evaluator(const Environment::Ptr& env): env_(env){}
    Value::Ptr get_result() { return std::move(result_); }

    ~Evaluator()=default;
private:
    void forNumber(const NumberE& num) override { result_ = std::make_unique<NumberV>(num.value_); }
    void forSymbol(const SymbolE& s) override;
    void forQuote(const Quote& quo) override{ result_ = quo.data_; }
    void forDefine(const Define& def) override;
    void forIf(const If&) override;
    void forLambda(const LambdaE &) override;
    void forApplication(const Application&) override;

    Value::Ptr result_;
    Environment::Ptr env_;
};

//this class is designed to clone lambda expression because it need to be shared/copied when
//construct a LambdaV while the AST tree node is exclusive(std::unique_ptr<Expr>)
class Cloner: public VisitorE {
public:
    static Expr::Ptr clone(const Expr& src) {
        static Cloner clone;
        src.accept(clone);
        return std::move(clone.copy_);
    }

private:
    void forNumber(const NumberE& num) override { copy_ = std::make_unique<NumberE>(num); }
    void forSymbol(const SymbolE& s) override { copy_ = std::make_unique<SymbolE>(s); }
    void forQuote(const Quote& quo) override{ copy_ = std::make_unique<Quote>(quo); }
    void forDefine(const Define& def) override;
    void forIf(const If&) override;
    void forLambda(const LambdaE &) override;
    void forApplication(const Application&) override;

    Expr::Ptr copy_;
};

class Stringfier: public VisitorV{
public:
    static std::string&& to_string(const Value& val) {
        static Stringfier printer;
        val.accept(printer);
        return std::move(printer.str_);
    }

    ~Stringfier()=default;
private:
    void forNumber(const NumberV& num) override { str_ = std::to_string(num.value_); }
    void forSymbol(const SymbolV& s) override { str_ = *s.ptr_; }
    void forAppliable(const Appliable& app) override{ 
        str_ = app.func_type_ == Appliable::Type::Lambda? "#<lambda>": "#<buildin-procedure>";
    }
    void forBoolean(const Boolean& b) override { str_ = b.value_? "#t": "#f"; }
    void forCons(const Cons&) override;

    std::string str_;
};


template<typename Expect>
class ValueChecker: public VisitorV {
public:

    static const Expect* get_ptr(const Value& val) { 
        static ValueChecker<Expect> checker;
        val.accept(checker);
        return checker.result_; 
    }
private:
    void forNumber(const NumberV& v) override { check(v); }
    void forSymbol(const SymbolV& v) override  {check(v); }
    void forAppliable(const Appliable& v) override { check(v); }
    void forCons(const Cons& v) override { check(v); }
    void forBoolean(const Boolean& v) override { check(v); }
    
    template<typename Type>
    void check(const Type& t) {
        if constexpr (std::is_same_v<Type, Expect>) {
            result_ = &t;
        } else {
            throw std::invalid_argument(
                    std::string("expect ") + type_string<Expect>() + " got " + type_string<Type>());
        }
    }
    const Expect *result_;
};

class ValueEq: public VisitorV {
public:
    ValueEq(const Value* p):lhs_(p) {}
    static bool eq(const Value *lhs, const Value *rhs) {
        ValueEq comp(lhs);
        rhs->accept(comp);
        return comp.eq_;
    }

private:
    void forNumber(const NumberV& v) override { eq(v); }
    void forSymbol(const SymbolV& v) override  { eq(v); }
    void forAppliable(const Appliable& v) override { eq(v); }
    void forCons(const Cons& v) override { eq(v); }
    void forBoolean(const Boolean& v) override { eq(v); }
    
    template<typename RType>
    void eq(const RType& rhs) {
        if(lhs_ == &rhs) { 
            eq_ = true; 
        }else if(lhs_->type_ == rhs.type_){
            if constexpr(std::is_same_v<RType, NumberV>) {
                eq_ = static_cast<const NumberV*>(lhs_)->value_ == rhs.value_;
            }else if constexpr(std::is_same_v<RType, SymbolV>) {
                eq_ = static_cast<const SymbolV*>(lhs_)->ptr_ == rhs.ptr_;
            }else {
                eq_ = false;
            }
        }else {
            eq_ = false;
        }
    }
    const Value* lhs_;
    bool eq_;
};


std::vector<std::string> tokenize(const char *);
Expr::Ptr parse(std::vector<std::string>::iterator&, std::vector<std::string>::iterator&);
Value::Ptr parse_data(std::vector<std::string>::iterator&, std::vector<std::string>::iterator&);

const std::string* make_symbol(const std::string&);
bool check_args_exact(const Appliable::Arg&, size_t);
bool check_args_at_least(const Appliable::Arg&, size_t);
