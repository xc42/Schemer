#ifndef __TOYLISP__H
#define __TOYLISP__H

#include <memory>
#include <map>
#include <vector>
#include <functional>

struct Environment;
struct Expr
{
    typedef std::shared_ptr<Expr> Ptr;

    enum class Type {Number, Symbol, List, Lambda, Procedure};

    Type type;

    Expr()=default;
    Expr(Type t):type(t){}
    virtual std::string to_string()=0;
    virtual Ptr eval(std::shared_ptr<Environment> env)=0;
    virtual ~Expr()=default;
};

struct Number:Expr, std::enable_shared_from_this<Number>
{
    long value;

    Number():Expr(Expr::Type::Number){}
    Number(const std::string &str):Expr(Expr::Type::Number),value(std::stol(str)){}
    Number(long v):Expr(Expr::Type::Number),value(v){}
    
    Expr::Ptr eval(std::shared_ptr<Environment> env){
        return shared_from_this();
    }

    std::string to_string() {
        return std::to_string(value);
    }
};

struct Symbol:public Expr
{
    typedef std::shared_ptr<Symbol> Ptr;

    std::string name;

    Symbol():Expr(Expr::Type::Symbol){}
    Symbol(const std::string &str):Expr(Expr::Type::Symbol),name(str){}

    Expr::Ptr eval(std::shared_ptr<Environment> env);
    std::string to_string() {return name;}
};

struct List:public Expr, std::enable_shared_from_this<List>
{
    typedef std::shared_ptr<List> Ptr;

    Expr::Ptr expr; //null if this is an empty list
    Expr::Ptr next;

    List():Expr(Expr::Type::List), expr(nullptr), next(nullptr){}
    List(Expr::Ptr _expr, Expr::Ptr _next):
        Expr(Expr::Type::List), expr(_expr), next(_next){}

    Expr::Ptr eval(std::shared_ptr<Environment> env);
    List::Ptr next_list(){return std::static_pointer_cast<List>(next);}

    bool length_equals(long len); 
    long length();
    std::string to_string(); 

    static bool can_iterate(List::Ptr lst) {
        return lst != nullptr && lst->expr != nullptr;
    }
};

struct Lambda:public Expr, std::enable_shared_from_this<Lambda>
{
    List::Ptr params;
    List::Ptr body;
    std::shared_ptr<Environment> env;

    Lambda():Expr(Expr::Type::Lambda){}
    Lambda(List::Ptr p, List::Ptr b, std::shared_ptr<Environment> e):
           Expr(Expr::Type::Lambda), params(p), body(b), env(e){}

    Expr::Ptr eval(std::shared_ptr<Environment> env){
        return shared_from_this();
    }

    std::string to_string() {return std::string("<Lambda>");}
};

//primitive procedures
//if we want to make primitve procedures like "cons" "+" "-"...
//behave like a function object(ie: "(define add +)"),we need to add them to evaluation 
//environment, otherwise they will be treat as keyword like "if" "quote"
struct Procedure:public Expr, std::enable_shared_from_this<Procedure>
{
    typedef std::function<Expr::Ptr(List::Ptr,std::shared_ptr<Environment>)> ProcType;

    ProcType proc;
    Procedure():Expr(Expr::Type::Procedure){}
    Procedure(const ProcType &p):Expr(Expr::Type::Procedure), proc(p){}

    Expr::Ptr eval(std::shared_ptr<Environment> env) {
        return shared_from_this();
    }
    std::string to_string(){return "<Primitive Procedure>";}
};

struct Environment:public std::enable_shared_from_this<Environment>
{
    typedef std::shared_ptr<Environment> Ptr;
    typedef std::map<std::string, Expr::Ptr> Table;

    Table symtable;
    Ptr   outer_env;
   
    Environment()=default;
    Environment(List::Ptr vars, List::Ptr vals) {add_bindings(vars, vals);}
    Environment(const Table &pairs):symtable(pairs){}

    void add_bindings(List::Ptr vars, List::Ptr vals);
    void add_bindings(const Table &pairs); 
    void add_bindings(const std::string &name, Expr::Ptr expr) {
        symtable.insert({name, expr});
    }

    Expr::Ptr find_value(const std::string &name); 
    Ptr extended_env(List::Ptr vars, List::Ptr vals) {
        auto new_env = std::make_shared<Environment>(vars, vals);
        new_env->outer_env = shared_from_this(); 
        return new_env;
    }
};

//globals
extern const std::shared_ptr<Symbol> LISP_TRUE, LISP_FALSE;


void check_arguments(List::Ptr args, long arg_len,const std::string &funcname);
void check_arguments(Expr::Ptr expr, Expr::Type type, const std::string& funcname);
void check_arguments(List::Ptr args, Expr::Type type, const std::string& funcname);

List::Ptr eval_arguments(List::Ptr args, Environment::Ptr env);
//primitive procedures
Expr::Ptr eval_quote(List::Ptr args, Environment::Ptr env); 
Expr::Ptr eval_cons(List::Ptr args, Environment::Ptr env); 
Expr::Ptr eval_car(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_cdr(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_if(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_lambda(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_define(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_list(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_and(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_or(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_not(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_nullq(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_eqq(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_pairq(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_numberq(List::Ptr args, Environment::Ptr env);
Expr::Ptr eval_symbolq(List::Ptr args, Environment::Ptr env);

Expr::Ptr apply(Expr::Ptr func, List::Ptr args, Environment::Ptr env);

#endif
