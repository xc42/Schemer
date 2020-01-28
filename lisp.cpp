#include "lisp.h"
#include "builtins.hpp"
#include <unordered_set>
#include <memory>
#include <cctype>
#include <cassert>
#include <iostream>
#include <sstream>

void NumberE::accept(VisitorE& v) const { v.forNumber(*this); }
void SymbolE::accept(VisitorE& v) const { v.forSymbol(*this); }
void Quote::accept(VisitorE& v) const { v.forQuote(*this); }
void Define::accept(VisitorE& v) const { v.forDefine(*this); }
void If::accept(VisitorE& v) const { v.forIf(*this); }
void LambdaE::accept(VisitorE& v) const { v.forLambda(*this); }
void Application::accept(VisitorE& v) const { v.forApplication(*this); }

void NumberV::accept(VisitorV& v) const { v.forNumber(*this); }
void SymbolV::accept(VisitorV& v) const { v.forSymbol(*this); }
void LambdaV::accept(VisitorV& v) const { v.forAppliable(*this); }
void Procedure::accept(VisitorV& v) const { v.forAppliable(*this); }
void Cons::accept(VisitorV& v) const { v.forCons(*this); }
void Boolean::accept(VisitorV& v) const { v.forBoolean(*this); }

void Environment::bind(const SymbolE& name, const Value::Ptr& val) { 
    bindings_[name] = val; 
    //debug:
    //std::cout << "binded variable " <<  "`" << *name.ptr_ << "`" <<
    //    (val==nullptr?"":" with value " + Stringfier::to_string(*val)) <<  " in env " << this << '\n';
}

Environment::Ptr Environment::extend(const LambdaE::ParamsType& names, const Appliable::Arg& values, const Environment::Ptr& old) {
    auto new_env = std::make_shared<Environment>();
    new_env->outer_ = old;
    check_args_exact(values, names.size());
    for(size_t i = 0; i < names.size(); ++i)
        new_env->bind(names[i], values[i]);

    return new_env;
}

void Evaluator::forSymbol(const SymbolE& s) {
    auto val = (*env_)(s);
    if(val.has_value()) {
        result_ = val.value();
    }else {
        throw std::invalid_argument{"#fatal: unbound variable: " + std::string(*s.ptr_)};
    }
}

void Evaluator::forDefine(const Define &def) {
    def.body_->accept(*this);
    env_->bind(def.name_, result_);
}

void Evaluator::forIf(const If &if_expr) {
    if_expr.pred_->accept(*this);
    if(*std::dynamic_pointer_cast<Boolean>(result_)) {
        if_expr.conseq_->accept(*this);
    }else {
        if_expr.alter_->accept(*this);
    }
}

void Evaluator::forLambda(const LambdaE &lambda) {
    auto lambda_cp = static_cast<LambdaE*>(Cloner::clone(lambda).release());
    result_ = std::make_shared<LambdaV>(std::unique_ptr<LambdaE>(lambda_cp), env_);
}

void Evaluator::forApplication(const Application &app) {
    app.operator_->accept(*this);
    auto rator = std::move(result_);

    std::vector<Value::Ptr> rands;
    for(auto &rand: app.operands_) {
        rand->accept(*this);
        rands.emplace_back(result_);
    }
    result_ = ValueChecker<Appliable>::get_ptr(*rator)->apply(rands);
}

Value::Ptr LambdaV::apply(const Appliable::Arg& args) const {
    auto new_env = Environment::extend(expr_->params_, args, env_);
    Evaluator eval{new_env};
    //return Evaluator::eval(expr_->body_, new_env);
    expr_->body_->accept(eval);
    return eval.get_result();
}


void Cloner::forDefine(const Define& def) {
    def.body_->accept(*this); //copy body
    auto body_cp = std::move(copy_);
    copy_ = std::make_unique<Define>(def.name_, std::move(body_cp));
}

void Cloner::forIf(const If& ifexp) {
    ifexp.pred_->accept(*this);
    auto pred_cp = std::move(copy_);
    ifexp.conseq_->accept(*this);
    auto conseq_cp = std::move(copy_);
    ifexp.alter_->accept(*this);
    auto alter_cp = std::move(copy_);
    copy_ = std::make_unique<If>(std::move(pred_cp), std::move(conseq_cp), std::move(alter_cp));
}

void Cloner::forLambda(const LambdaE& lambda) {
    lambda.body_->accept(*this);
    copy_ = std::make_unique<LambdaE>(lambda.params_, std::move(copy_));
}

void Cloner::forApplication(const Application& app) {
    app.operator_->accept(*this);
    auto rator = std::move(copy_);

    Application::Operands rands;
    for(const auto& rand: app.operands_) {
        rand->accept(*this);
        rands.emplace_back(std::move(copy_));
    }
    copy_ = std::make_unique<Application>(std::move(rator), std::move(rands));
}

void Stringfier::forCons(const Cons& cc) {
    std::string res ;
    res += "(";
    const Cons* it = &cc;
    while(it) {
        if(it->car_) {
            it->car_->accept(*this);
            res += str_;
        }else {
            res += "()";
        }

        if(!it->cdr_) break; //no second element

        res += " ";
        if(it->cdr_->type_ == Value::Type::Cons) {
            it = static_cast<Cons*>(it->cdr_.get());
        }else {
            res += ". ";
            it->cdr_->accept(*this);
            res += str_;
            break;
        }
    }
    res += ")";
    str_ = std::move(res);
}

bool check_args_exact(const Appliable::Arg& args, size_t expect) {
    if(args.size() != (size_t)expect) {
        std::stringstream ss;
        ss << "expect " <<  expect << " arguments, got " << args.size();
        throw std::length_error(ss.str());
    }
    return true;
}

bool check_args_at_least(const Appliable::Arg& args, size_t least) {
    if(args.size() < least) {
        std::stringstream ss;
        ss << "expect at least " << least << " arguments, got " << args.size();
        throw std::length_error(ss.str());
    }
    return true;
}

bool check_arg(const Value::Ptr& arg, Value::Type type) {
    static const char* type_str[] = {"Number", "Symbol", "Appliable", "Cons", "Boolean"};
    using Type = std::underlying_type_t<Value::Type>;
    if(arg->type_ != type) {
        throw std::invalid_argument("expect " + 
                std::string(type_str[static_cast<Type>(type)]) + 
                " but got " + type_str[static_cast<Type>(arg->type_)]);
    }
    return true;
}

std::vector<std::string> tokenize(const char *c)
{
    std::vector<std::string> tokens;
    while(*c != '\0') {
       while(*c != '\0' && std::isspace(*c)) ++c;
       if(*c == '\0') break;

       switch(*c) {
           case '(':
               tokens.push_back(std::string(1, *c++));
               break;
           case ')':
               tokens.push_back(std::string(1, *c++));
               break;
           case '\'':
               tokens.push_back(std::string(1, *c++));
               break;
           default:
               const char *c2 = c++;
               while(*c != '\0' && *c != '(' && *c != ')' && !std::isspace(*c)) ++c;
               tokens.push_back(std::string(c2, c));
       }
    }
    return tokens;
}

const std::string* add_symbol(const std::string& s) {
    static std::unordered_set<std::string> string_intern;
    auto pos = string_intern.insert(s).first;
    return &*pos;
}

Expr::Ptr parse(std::vector<std::string>::iterator &start,
                std::vector<std::string>::iterator &end)
{
    if(start == end)  return nullptr;

    const auto &str = *start;
    //numbers
    if(std::isdigit(str[0]) || (str[0] == '-' && std::isdigit(str[1]))) {
        return std::make_unique<NumberE>(std::stoll(*start++)); //
    }
    //quote
    else if(*start == "'") {
        ++start;
        auto quote = parse_data(start, end);
        return std::make_unique<Quote>(std::move(quote));
    }
    //list
    else if(*start == "(") {
        ++start;
        if(*start == "define") {
            ++start;
            auto name = parse(start, end);
            auto body = parse(start, end);
            ++start;
            return std::make_unique<Define>(*dynamic_cast<SymbolE*>(name.get()), std::move(body));
        }else if(*start == "if") {
            ++start;
            auto pred = parse(start, end);
            auto cons = parse(start, end);
            auto alter = parse(start, end);
            ++start;
            return std::make_unique<If>(std::move(pred), std::move(cons), std::move(alter));
        }else if(*start == "lambda") {
            ++start;
            assert(*start == "(");
            ++start;

            LambdaE::ParamsType params;
            while(*start != ")") {
                auto param = parse(start, end);
                params.emplace_back(*dynamic_cast<SymbolE*>(param.get()));
            }
            ++start; //skip ")"
            auto body = parse(start, end);
            ++start;
            return std::make_unique<LambdaE>(std::move(params), std::move(body));
        }else if(*start == ")") {
            throw std::invalid_argument("empty application");
        }else { //application
            auto rator = parse(start, end);
            Application::Operands rands;
            while(*start != ")") {
                rands.emplace_back(parse(start, end));
            }
            ++start;
            return std::make_unique<Application>(std::move(rator), std::move(rands));
        }
    }
    //symbol
    else {
        return std::make_unique<SymbolE>(add_symbol(*start++));
    }
}

//parse the program text as data
Value::Ptr parse_data(std::vector<std::string>::iterator &start,
                std::vector<std::string>::iterator &end) 
{
    if(start == end)  return nullptr;

    const auto& str = *start;
    if(std::isdigit(str[0]) || (str[0] == '-' && std::isdigit(str[1]))) {
        return std::make_shared<NumberV>(std::stoll(*start++)); //
    }
    else if(*start == "(") {
        ++start;
        Cons head;
        Value* it = &head;
        while(*start != ")") {
            auto new_cell = std::make_shared<Cons>(parse_data(start, end));
            static_cast<Cons*>(it)->cdr_ = new_cell;
            it = new_cell.get();
        }
        ++start;
        return head.cdr_;
    }
    else {
        return std::make_shared<SymbolV>(add_symbol(*start++));
    }
}

//read-eval-print-loop
void repl(const Environment::Ptr env) {
    const std::string input_promt{"(toyLisp)~> "};
    Evaluator eval{env};
    for(;;) {
        std::cout << input_promt;
        std::string line;
        int n_left_paren = 0;
        do {
            std::string next_line;
            std::getline(std::cin, next_line);

            if(std::cin.eof()) {
                std::cout << "\n\nMoriturus te saluto.\n" << std::endl;
                return ;
            }
                
            for(const char c: next_line) {
                if(c == '(') ++n_left_paren;
                else if(c == ')') --n_left_paren;
            }
            line += next_line;
        }while(n_left_paren > 0);

        if(n_left_paren != 0) {
            std::cout << "parentheses mismatch!\n";
            continue;
        }

        Expr::Ptr expr;
        try { //parse
            auto tokens = tokenize(line.c_str());
            auto start = tokens.begin(), end = tokens.end();
            expr =  parse(start, end);
        }catch(std::exception& e) {
            std::cout << e.what() << '\n';
        }

        if(expr != nullptr) {
            try { //eval
                expr->accept(eval);
                std::cout << Stringfier::to_string(*eval.get_result()) << "\n" << std::endl;
            }
            catch(std::exception &e) {
                std::cout << e.what() << "\n" ;
            }
        }
    }
}

Environment::Ptr setup_env() {
    auto initial_env = std::make_shared<Environment>();
    //constant
    initial_env->bind(add_symbol("#t"), std::make_shared<Boolean>(true));
    initial_env->bind(add_symbol("#f"), std::make_shared<Boolean>(false));

    using namespace builtin;
    //arithmetic functions
    initial_env->bind(add_symbol("+"), std::make_shared<Procedure>(Arith<std::plus<NumberV::Type>>()));
    initial_env->bind(add_symbol("-"), std::make_shared<Procedure>(Arith<std::minus<NumberV::Type>>()));
    initial_env->bind(add_symbol("*"), std::make_shared<Procedure>(Arith<std::multiplies<NumberV::Type>>()));
    initial_env->bind(add_symbol("/"), std::make_shared<Procedure>(Arith<std::divides<NumberV::Type>>()));

    //loagical functions*
    initial_env->bind(add_symbol("="), std::make_shared<Procedure>(Comparator<std::equal_to<NumberV::Type>>()));
    initial_env->bind(add_symbol("<"), std::make_shared<Procedure>(Comparator<std::less<NumberV::Type>>()));
    initial_env->bind(add_symbol("<="), std::make_shared<Procedure>(Comparator<std::less_equal<NumberV::Type>>()));
    initial_env->bind(add_symbol(">"), std::make_shared<Procedure>(Comparator<std::greater<NumberV::Type>>()));
    initial_env->bind(add_symbol(">="), std::make_shared<Procedure>(Comparator<std::greater_equal<NumberV::Type>>()));

    initial_env->bind(add_symbol("cons"), std::make_shared<Procedure>(cons));
    initial_env->bind(add_symbol("car"), std::make_shared<Procedure>(car));
    initial_env->bind(add_symbol("cdr"), std::make_shared<Procedure>(cdr));
    initial_env->bind(add_symbol("null?"), std::make_shared<Procedure>(nullq));
    initial_env->bind(add_symbol("eq?"), std::make_shared<Procedure>(eqq));

    return initial_env;
}

int main()
{
    repl(setup_env());
}
