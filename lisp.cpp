#include <exception>
#include <iostream>
#include <cctype>
#include "lisp.h"

//global lisp symbol 
const std::shared_ptr<Symbol> LISP_FALSE = std::make_shared<Symbol>("#f"); 
const std::shared_ptr<Symbol> LISP_TRUE = std::make_shared<Symbol>("#t"); //anything not false is true

Expr::Ptr Symbol::eval(Environment::Ptr env){
    return env->find_value(name);
}

Expr::Ptr List::eval(Environment::Ptr env)
{
    if(can_iterate(shared_from_this()))
        return apply(expr, next_list(), env);
    else
        return shared_from_this();
}

bool List::length_equals(long len) 
{
    if(len < 0) return false;
    if(expr == nullptr)
        return 0 == len;
    else {
        if(next == nullptr)
            return 1 == len;
        else
            return next_list()->length_equals(len-1);
    }
}

long List::length() {
    if(expr == nullptr)
        return 0;
    else {
        if(next == nullptr)
            return 1;
        else 
            return 1 + next_list()->length();
    }
}

std::string List::to_string() {
    std::string str{"("};
    auto iter = shared_from_this();
    while(can_iterate(iter)) {
        str += iter->expr->to_string();
        if(iter->next != nullptr) {
            if(next->type != Expr::Type::List) { //cons cell
                str += " . " + next->to_string();
                break;
            }
            else { //succesive list
                if(can_iterate(iter->next_list())) {
                    str += " ";
                    iter = iter->next_list();
                }
                else //next is empty list whoes expr is null
                    break;
            }
        }
        else //next is nullptr
            break;
    }
    str += ")";
    return str;
}

void Environment::add_bindings(List::Ptr vars, List::Ptr vals) 
{
    while(List::can_iterate(vars) && List::can_iterate(vals)) {
        symtable.insert({std::static_pointer_cast<Symbol>(vars->expr)->name,
                         vals->expr});

        vars = vars->next_list();
        vals = vals->next_list();
    }
    if(List::can_iterate(vars) || List::can_iterate(vals))
        throw std::invalid_argument("Number of parameters and arguments don't match!\n");
}

void Environment::add_bindings(const Table &pairs)
{
    for(const auto &var_val: pairs) 
        symtable.insert(var_val);
}

Expr::Ptr Environment::find_value(const std::string &name) 
{
    auto iter = symtable.find(name);
    if(iter == symtable.end()) {
        if(outer_env != nullptr) 
            return outer_env->find_value(name);
        else
            return nullptr;
    }
    return iter->second;
}


Expr::Ptr apply(Expr::Ptr func, List::Ptr args, Environment::Ptr env)
{
    static const std::map<std::string, Procedure::ProcType> primitives 
    {{"if", eval_if}, {"lambda", eval_lambda}, 
     {"quote", eval_quote}, {"define", eval_define},
     {"and", eval_and}, {"or", eval_or}};

    switch(func->type) {
        case Expr::Type::Lambda:
            {
                auto lam = std::static_pointer_cast<Lambda>(func);
                auto sub_body = lam->body;
                auto new_env = lam->env->extended_env(lam->params,
                                                      eval_arguments(args, env));
                Expr::Ptr value;
                while(List::can_iterate(sub_body)) {
                   value = sub_body->expr->eval(new_env);
                   sub_body = sub_body->next_list();
                }
                return value; 
            }
        case Expr::Type::Symbol:
            {
                auto sym = std::static_pointer_cast<Symbol>(func);
                auto proc = primitives.find(sym->name);
                if(proc != primitives.end()) {
                    return proc->second(args, env);
                }
                else{
                    auto proc = env->find_value(sym->name);
                    if(proc != nullptr) 
                        return apply(proc, args, env);
                    else
                        throw std::invalid_argument(sym->name+" not found!\n");
                }
            }
            break;
        case Expr::Type::Procedure:
            return std::static_pointer_cast<Procedure>(func)->proc(args, env);
        case Expr::Type::Number:
            throw std::invalid_argument("Number is not appliable");
            break;
        case Expr::Type::List:
            return apply(func->eval(env), args, env);
    }
    return nullptr;
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

Expr::Ptr parse(std::vector<std::string>::iterator &start,
                std::vector<std::string>::iterator &end)
{
    //for a symbol, if already defined, we return pointer,
    //so compare symbol only compare their pointer,else
    //we create new
    static std::map<std::string, Symbol::Ptr> sym_pools
    {{"#t",LISP_TRUE}, {"#f", LISP_FALSE},
     {"quote", std::make_shared<Symbol>("quote")}};

    if(start != end) {
        const auto &str = *start;
        //numbers
        if(std::isdigit(str[0]) || (str[0] == '-' && std::isdigit(str[1]))) 
        { 
            return std::make_shared<Number>(std::stol(*start++));
        }
        //list
        else if(*start == "(") {
            ++start;
            auto lst = std::make_shared<List>();
            auto iter = lst;
            while(*start != ")") {
                iter->expr = parse(start, end);
                //list tail will contain an empty list whose expr is null
                iter->next = std::make_shared<List>();
                iter = iter->next_list();
            }
            ++start;
            return lst; 
        }
        //quote
        else if(*start == "'") {
            ++start;
            auto quote_expr = parse(start, end);
            return std::make_shared<List>(sym_pools["quote"],
                   std::make_shared<List>(quote_expr,std::make_shared<List>()));
        }
        //symbol
        else {
            auto it = sym_pools.find(*start);
            if(it != sym_pools.end()) {
                ++start;
                return it->second;
            }
            else { 
                const auto& news = std::make_shared<Symbol>(*start);
                sym_pools.insert({*start, news});
                ++start;
                return news;
            }
        }
    }
    return nullptr; //no input so no expr
}

template<typename OpType>
struct Arithmetic{
    OpType op;
    Expr::Ptr operator()(List::Ptr args, Environment::Ptr env)
    {
        auto arg_val = eval_arguments(args, env);
        check_arguments(arg_val, Expr::Type::Number, "Arithmetic");
        std::vector<long> nums;
        while(List::can_iterate(arg_val)) {
            nums.push_back(std::static_pointer_cast<Number>(arg_val->expr)->value);
            arg_val = arg_val->next_list();
        }
        long ans = nums[0];
        for(auto n = nums.begin()+1; n != nums.end(); ++n)
            ans = op(ans, *n);
        return std::make_shared<Number>(ans);
    }
};

template<typename OpType>
struct Comparator
{
    OpType op;
    Expr::Ptr operator()(List::Ptr args, Environment::Ptr env) 
    {
        check_arguments(args, 2, "logical compare");
        auto arg_val = eval_arguments(args, env);
        check_arguments(arg_val, Expr::Type::Number, "logical compare");
        auto e1 = arg_val->expr;
        auto e2 = std::static_pointer_cast<List>(arg_val->next)->expr;
        long v1 = std::static_pointer_cast<Number>(e1)->value;
        long v2 = std::static_pointer_cast<Number>(e2)->value;
        return op(v1, v2) ? LISP_TRUE: LISP_FALSE;
    }

};

std::shared_ptr<Environment> init_global_env()
{
    static const std::map<std::string, Expr::Ptr>  
    globals{
    {"#t", LISP_TRUE},
    {"#f", LISP_FALSE},
    {"car", std::make_shared<Procedure>(eval_car)}, 
    {"cdr", std::make_shared<Procedure>(eval_cdr)}, 
    {"cons", std::make_shared<Procedure>(eval_cons)},
    {"list", std::make_shared<Procedure>(eval_list)},
    {"eq?", std::make_shared<Procedure>(eval_eqq)},
    {"null?", std::make_shared<Procedure>(eval_nullq)},
    {"pair?", std::make_shared<Procedure>(eval_pairq)},
    {"number?", std::make_shared<Procedure>(eval_numberq)},
    {"symbol?", std::make_shared<Procedure>(eval_symbolq)},
    {"not", std::make_shared<Procedure>(eval_not)},
    {"+", std::make_shared<Procedure>(Arithmetic<std::plus<long>>())}, 
    {"-", std::make_shared<Procedure>(Arithmetic<std::minus<long>>())},
    {"*", std::make_shared<Procedure>(Arithmetic<std::multiplies<long>>())}, 
    {"/", std::make_shared<Procedure>(Arithmetic<std::divides<long>>())},
    {"%", std::make_shared<Procedure>(Arithmetic<std::modulus<long>>())},
    {"<", std::make_shared<Procedure>(Comparator<std::less<long>>())}, 
    {">", std::make_shared<Procedure>(Comparator<std::greater_equal<long>>())},
    {"=", std::make_shared<Procedure>(Comparator<std::equal_to<long>>())},
    {">=", std::make_shared<Procedure>(Comparator<std::greater_equal<long>>())},
    {"<=", std::make_shared<Procedure>(Comparator<std::less_equal<long>>())},
    };

    return std::make_shared<Environment>(globals);
}

//read-eval-print-loop
void repl()
{
    auto user_initial_env = init_global_env();
    const std::string input_promt{"(toyLisp)~> "};
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
                auto result = expr->eval(user_initial_env);
                std::cout << "\n;value:\t" << result->to_string();
            }
            catch(std::exception &e) {
                std::cout << "\n" << e.what();
            }
        }
        std::cout << "\n" << std::endl;
    }
}

int main()
{
    repl();
}
