#pragma once

#include <vector>
#include <map>
#include <functional>
#include <memory>

class VisitorE; //forward declaration for base visitor

//Expr = NumberE| SymbolE| Quote| Define| If| Lambda| Application
struct Expr {
    using Ptr = std::unique_ptr<Expr>;

    enum class Type {Number, Symbol, Quote, Define, If, Lambda, Application} type_;

    Expr(Type t):type_(t){}
    virtual void accept(VisitorE& visitor) = 0;
    virtual ~Expr()=default;
};

struct NumberE: Expr{
    using Type = long long;
    NumberE(const std::string& s): Expr(Expr::Type::Number), value_(std::stoll(s)){};
    void accept(VisitorE &v) override;

    Type value_;
};

struct SymbolE: Expr{
    SymbolE(const std::string* p):Expr(Expr::Type::Symbol), ptr_(p){}
    void accept(VisitorE &v) override;
    bool operator<(const SymbolE& s) const { return ptr_ < s.ptr_; }
    const std::string *ptr_;    
};


struct Quote: Expr{
    Quote(Expr::Ptr &&e):Expr(Expr::Type::Quote), expr_(std::move(e)){}
    void accept(VisitorE &v) override;

    Expr::Ptr expr_;
};


struct Define: Expr {
    Define(std::unique_ptr<SymbolE>&& n, Expr::Ptr &&b):
        Expr(Expr::Type::Define), name_(std::move(n)), body_(std::move(b)){}

    void accept(VisitorE &v) override;

    std::unique_ptr<SymbolE> name_;
    Expr::Ptr body_;
};

struct If: Expr{
    If(Expr::Ptr &&p, Expr::Ptr &&c, Expr::Ptr &&a):
        Expr(Expr::Type::If), pred_(std::move(p)), conseq_(std::move(c)), alter_(std::move(a)){}

    void accept(VisitorE &v) override;

    Expr::Ptr pred_, conseq_, alter_;
};

struct LambdaE: Expr{
    using ParamsType = std::vector<std::unique_ptr<SymbolE>>;
    LambdaE(ParamsType&& params, Expr::Ptr&& b):
        Expr(Expr::Type::Lambda), params_(std::move(params)), body_(std::move(b)){}

    void accept(VisitorE &v) override;

    ParamsType params_;
    Expr::Ptr body_;
};

struct Application: Expr{
    using Operands = std::vector<Expr::Ptr>;
    Application(Expr::Ptr &&rator, Operands&& rands):
        Expr(Expr::Type::Application), operator_(std::move(rator)), operands_(std::move(rands)){}

    void accept(VisitorE &v) override;

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
    virtual void accept(VisitorV& visitor)=0;
    virtual ~Value()=default;
};

struct NumberV: public Value {
    using Type = long long;
    NumberV(Type v):Value(Value::Type::Number), value_(v){};
    void accept(VisitorV &v) override;
    ~NumberV()=default;

    Type value_;
};

struct SymbolV: public Value {
    SymbolV(const SymbolE& s):Value(Value::Type::Symbol), ptr_(s.ptr_){}
    void accept(VisitorV &v) override;
    bool operator<(const SymbolV& s) const { return ptr_ < s.ptr_; }
    ~SymbolV()=default;

    const std::string *ptr_; //do not need free, handled elsewhere
};

struct Appliable: public Value {
    using Arg = std::vector<Value::Ptr>;
    enum class Type { Lambda, BuildIn } type_;

    Appliable(Type t):Value(Value::Type::Appliable), type_(t){}
    virtual Value::Ptr apply(Arg&)=0;
    virtual ~Appliable()=default;
};

class Environment;
struct LambdaV: public Appliable{
    LambdaV(LambdaE&& e, const std::shared_ptr<Environment>& env):
        Appliable(Appliable::Type::Lambda), expr_(std::move(e)), env_(env) {}

    void accept(VisitorV &v) override;
    Value::Ptr apply(Appliable::Arg&) override;

    LambdaE expr_;
    std::shared_ptr<Environment> env_;
};

class Procedure: public Appliable {
public:
    using Func = std::function<Value::Ptr(Appliable::Arg&)>;
    Procedure(const Func& f):Appliable(Appliable::Type::BuildIn), func(f){}

    void accept(VisitorV &v) override;
    Value::Ptr apply(Appliable::Arg& args) override { return func(args); }
    ~Procedure()=default;
private:
    Func func;
};

struct Cons: public Value {
    Cons(const Value::Ptr& car, const Value::Ptr& cdr):
        Value(Value::Type::Cons), car_(car), cdr_(cdr) {}

    void accept(VisitorV &v) override;
    ~Cons()=default;
    Value::Ptr car_, cdr_;
};

struct Boolean: public Value {
    Boolean(bool b): Value(Value::Type::Boolean), value_(b) {}
    void accept(VisitorV &v) override;
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

    void bind(const SymbolE& name, const Value::Ptr& val) { bindings_.emplace(name, val); }
    static Ptr extend(const LambdaE::ParamsType&, Appliable::Arg&, const Ptr& old);

    Value::Ptr operator()(const SymbolE& s) { 
        auto it = bindings_.find(s);
        return it != bindings_.end()? it->second: (*outer_)(s);
    }

private:
    std::map<SymbolE, Value::Ptr> bindings_;
    Ptr outer_;
};


class VisitorE {
public:
    virtual void forNumber(NumberE&)=0;
    virtual void forSymbol(SymbolE&)=0;
    virtual void forQuote(Quote&)=0;
    virtual void forDefine(Define&)=0;
    virtual void forIf(If&)=0;
    virtual void forLambda(LambdaE&)=0;
    virtual void forApplication(Application&)=0;
    virtual ~VisitorE()=default;
};

class Evaluator: public VisitorE {
public:
    Evaluator():env_(std::make_shared<Environment>()){}
    Evaluator(const Environment::Ptr& env): env_(env){}
    void forNumber(NumberE& num) override { result_ = std::make_unique<NumberV>(num.value_); }
    void forSymbol(SymbolE& s) override;
    void forQuote(Quote& ) override{ }//TODO 
    void forDefine(Define& def) override;
    void forIf(If&) override;
    void forLambda(LambdaE &) override;
    void forApplication(Application&) override;

    Value::Ptr get_result(){ return result_; }
    ~Evaluator()=default;
private:
    Value::Ptr result_;
    Environment::Ptr env_;
};

class VisitorV {
public:
    virtual void forNumber(NumberV&)=0;
    virtual void forSymbol(SymbolV&)=0;
    virtual void forAppliable(Appliable&)=0;
    virtual void forCons(Cons&)=0;
    virtual void forBoolean(Boolean&)=0;
    virtual ~VisitorV()=default;
};


class Printer: public VisitorV{
public:
    void forNumber(NumberV& num) override { str = std::to_string(num.value_); }
    void forSymbol(SymbolV& s) override { str = *s.ptr_; }
    void forAppliable(Appliable& app) override{ 
        str = app.type_ == Appliable::Type::Lambda? "<#lambda>": "<#buildin-procedure>";
    }
    void forBoolean(Boolean& b) override { str = b.value_? "#t": "#f"; }
    void forCons(Cons&) override{};

    const std::string& get_result() { return str; }
    ~Printer()=default;
private:
    std::string str;
};

const std::string* make_symbol(const std::string&);
bool check_args_exact(const Appliable::Arg&, size_t);
bool check_args_at_least(const Appliable::Arg&, size_t);
bool check_arg(const Value::Ptr&, Value::Type);

Value::Ptr cons(Appliable::Arg&);
Value::Ptr car(Appliable::Arg&);
Value::Ptr cdr(Appliable::Arg&);

template<typename Expect>
class Checker: public VisitorV {
public:
    void forNumber(NumberV& num) override 
    { result_ = &num; }

    void forSymbol(SymbolV&) override  {
        throw std::invalid_argument(std::string("expect ") + 
            type_string<Expect>() + " got " + type_string<SymbolV>());
    }

    void forAppliable(Appliable&) override { 
        throw std::invalid_argument(std::string("expect ") + 
                type_string<Expect>() + " got " + type_string<Appliable>());
    }

    void forCons(Cons&) override {
        throw std::invalid_argument(std::string("expect ") + 
                type_string<Expect>() + " got " + type_string<Cons>() );
    }

    void forBoolean(Boolean&) override {
        throw std::invalid_argument(std::string("expect ") + 
                type_string<Expect>() + " got " + type_string<Boolean>());
    }

    Expect& get_result() { return *result_; }
private:
    Expect *result_;
};

template<typename ArithOp>
class Arith {
public:
    Arith():op_(ArithOp()){}
    Value::Ptr operator()(Appliable::Arg& args) {
        check_args_at_least(args, 2);
        args[0]->accept(num_extract);
        auto result = std::make_shared<NumberV>(num_extract.get_result().value_);
        for(auto i = ++args.begin(); i != args.end(); ++i) {
            (*i)->accept(num_extract);
            result->value_ = op_(result->value_, num_extract.get_result().value_);
        }
        return result;
    }
private:
    ArithOp op_;
    Checker<NumberV> num_extract;
};

template<typename LogicOp>
class Comparator {
public:
    Comparator():op_(LogicOp()){}
    Value::Ptr operator()(Appliable::Arg& args) {
        check_args_exact(args, 2);
        (*args.begin())->accept(num_extract);
        auto& lhs = num_extract.get_result();
        (*std::next(args.begin()))->accept(num_extract);
        auto& rhs = num_extract.get_result();
        return std::make_shared<Boolean>(op_(lhs.value_, rhs.value_));
    }
private:
    LogicOp op_;
    Checker<NumberV> num_extract;
};
