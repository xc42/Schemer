#include "lisp.h"
#include <unordered_set>
#include <memory>
#include <cctype>
#include <cassert>
#include <iostream>
#include <sstream>

void NumberE::accept(VisitorE& v) { v.forNumber(*this); }
void SymbolE::accept(VisitorE& v) { v.forSymbol(*this); }
void Quote::accept(VisitorE& v) { v.forQuote(*this); }
void Define::accept(VisitorE& v) { v.forDefine(*this); }
void If::accept(VisitorE& v) { v.forIf(*this); }
void LambdaE::accept(VisitorE& v) { v.forLambda(*this); }
void Application::accept(VisitorE& v) { v.forApplication(*this); }

void NumberV::accept(VisitorV& v) { v.forNumber(*this); }
void SymbolV::accept(VisitorV& v) { v.forSymbol(*this); }
void LambdaV::accept(VisitorV& v) { v.forAppliable(*this); }
void Procedure::accept(VisitorV& v) { v.forAppliable(*this); }
void Cons::accept(VisitorV& v) { v.forCons(*this); }
void Boolean::accept(VisitorV& v) { v.forBoolean(*this); }

Environment::Ptr Environment::extend(const LambdaE::ParamsType& names, Appliable::Arg& values, const Environment::Ptr& old) {
    auto new_env = std::make_shared<Environment>();
    new_env->outer_ = old;
    check_args_exact(values, names.size());
    for(size_t i = 0; i < names.size(); ++i)
        new_env->bind(*names[i], values[i]);

    return new_env;
}

void Evaluator::forSymbol(SymbolE& s) {
    result_ = (*env_)(s);
    if(result_ == nullptr) {
        throw std::invalid_argument{"unbounded variable: " + *s.ptr_};
    }
}

void Evaluator::forDefine(Define &def) {
    def.body_->accept(*this);
    env_->bind(*def.name_, result_);
}

void Evaluator::forIf(If &if_expr) {
    if_expr.pred_->accept(*this);
    assert(result_->type_ == Value::Type::Boolean);
    if(*std::static_pointer_cast<Boolean>(result_)) {
        if_expr.conseq_->accept(*this);
    }else {
        if_expr.alter_->accept(*this);
    }
}

void Evaluator::forLambda(LambdaE &lambda) {
    result_ = std::make_unique<LambdaV>(std::move(lambda), env_);
}

void Evaluator::forApplication(Application &app) {
    app.operator_->accept(*this);
    auto rator = std::dynamic_pointer_cast<Appliable>(result_);
    assert(rator != nullptr);
    
    std::vector<Value::Ptr> rands;
    for(auto &rand: app.operands_) {
        rand->accept(*this);
        rands.emplace_back(result_);
    }
    result_ = rator->apply(rands);
}

Value::Ptr LambdaV::apply(Appliable::Arg& args) {
    auto new_env = Environment::extend(expr_.params_, args, env_);
    Evaluator evaluator(new_env);
    expr_.body_->accept(evaluator);
    return evaluator.get_result();
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

    if(start != end) {
        const auto &str = *start;
        //numbers
        if(std::isdigit(str[0]) || (str[0] == '-' && std::isdigit(str[1]))) {
            return std::make_unique<NumberE>(*start++);
        }
        //quote
        else if(*start == "'") {
            ++start;
            auto quote = parse(start, end);
            return std::make_unique<Quote>(std::move(quote));
        }
        //list
        else if(*start == "(") {
            ++start;
            if(*start == "define") {
                ++start;
                auto name = parse(start, end);
                assert(name->type_ == Expr::Type::Symbol);
                auto body = parse(start, end);
                ++start;
                return std::make_unique<Define>(
                        std::unique_ptr<SymbolE>(static_cast<SymbolE*>(name.release())),
                        std::move(body));
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
                    assert(param->type_ == Expr::Type::Symbol);
                    params.emplace_back(std::unique_ptr<SymbolE>(static_cast<SymbolE*>(param.release())));
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
    return nullptr; //no input so no expr
}

//read-eval-print-loop
void repl(Evaluator &evaluator) {
    const std::string input_promt{"(toyLisp)~> "};
    Printer printer;
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
            std::cout << "parentheses not mathced!\n";
            continue;
        }

        auto tokens = tokenize(line.c_str());
        auto start = tokens.begin(), end = tokens.end();
        auto expr =  parse(start, end);

        if(expr != nullptr) {
            try {
                expr->accept(evaluator);
                evaluator.get_result()->accept(printer);
            }
            catch(std::exception &e) {
                std::cout << "\n" << e.what();
            }
        }
        std::cout << printer.get_result() << "\n" << std::endl;
    }
}

Evaluator setup() {
    auto initial_env = std::make_shared<Environment>();
    //constant
    initial_env->bind(add_symbol("#t"), std::make_shared<Boolean>(true));
    initial_env->bind(add_symbol("#f"), std::make_shared<Boolean>(false));

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

    return Evaluator{initial_env};
}

int main()
{
    Evaluator evaluator = setup();
    repl(evaluator);
}
