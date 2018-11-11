//argument handler 
#include "lisp.h"

void check_arguments(List::Ptr args, long arg_len,const std::string &funcname)
{
    if(!args->length_equals(arg_len))
        throw std::invalid_argument(funcname + " expects " + 
                                    std::to_string(arg_len)+
                                    " eval_arguments!\n");
}

void check_arguments(Expr::Ptr expr, Expr::Type type, const std::string& funcname)
{
    if(expr->type != type) 
        throw std::invalid_argument("argument of type "+ expr->to_string() +
                                    " passed to " + funcname + " is not a correct type!\n");
}

void check_arguments(List::Ptr args, Expr::Type type, const std::string& funcname)
{
    while(List::can_iterate(args)) {
        check_arguments(args->expr, type, funcname);
        args = args->next_list();
    }
}
                                    
List::Ptr eval_arguments(List::Ptr args, Environment::Ptr env)
{
    auto values = std::make_shared<List>();
    auto val_iter = values;
    while(List::can_iterate(args)) {
        val_iter->expr = args->expr->eval(env);
        val_iter->next = std::make_shared<List>();
        val_iter = val_iter->next_list();
        args = args->next_list();
    }
    return values;
}

//primitive procedures
Expr::Ptr eval_quote(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 1, "quote");
    return args->expr;
}

Expr::Ptr eval_cons(List::Ptr args, Environment::Ptr env) 
{
    check_arguments(args, 2, "cons");
    auto arg_val = eval_arguments(args, env);
    auto e1 = arg_val->expr, e2 = arg_val->next_list()->expr;
    return std::make_shared<List>(e1,e2);
}

Expr::Ptr eval_car(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 1, "car");
    auto arg_val = eval_arguments(args, env);
    check_arguments(arg_val->expr, Expr::Type::List, "car");
    return std::static_pointer_cast<List>(arg_val->expr)->expr;
}

Expr::Ptr eval_cdr(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 1, "cdr");
    auto arg_val = eval_arguments(args, env);
    check_arguments(arg_val->expr, Expr::Type::List, "cdr");
    return std::static_pointer_cast<List>(arg_val->expr)->next_list();
}

Expr::Ptr eval_list(List::Ptr args, Environment::Ptr env)
{
    return eval_arguments(args, env);
}

Expr::Ptr eval_if(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 3, "if");
    bool test = args->expr->eval(env) != LISP_FALSE;
    auto consequent = args->next_list();
    auto alternative = consequent->next_list();
    return test? consequent->expr->eval(env) : alternative->expr->eval(env);
}

Expr::Ptr eval_lambda(List::Ptr args, Environment::Ptr env)
{
    auto params = std::static_pointer_cast<List>(args->expr);
    auto body = args->next_list();
    return std::make_shared<Lambda>(params, body, env);
}

Expr::Ptr eval_define(List::Ptr args, Environment::Ptr env)
{
    Expr::Ptr ret;
    Expr::Ptr head = args->expr; 
    switch(head->type) {
        case Expr::Type::Symbol:
            {
                check_arguments(args, 2, "define");
                auto body = args->next_list()->expr;
                ret = body->eval(env);
                env->add_bindings(std::static_pointer_cast<Symbol>(head)->name,ret);
                return ret;
            }
        case Expr::Type::List: //syntactic sugar
            {
                auto head_lst = std::static_pointer_cast<List>(head); 
                auto param = head_lst->next_list();
                auto body = args->next_list();
                auto ret = std::make_shared<Lambda>(param, body, env);
                env->add_bindings(std::static_pointer_cast<Symbol>(head_lst->expr)
                                 ->name, ret);
                return ret;
            }
        default:
            throw std::invalid_argument("define need symbol!\n");
    }
    return nullptr;
}

Expr::Ptr eval_and(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 2, "and");
    auto e1 = args->expr->eval(env);
    if(e1 != LISP_FALSE) {
        auto e2 = std::static_pointer_cast<List>(args->next)->expr->eval(env);
        return e2 != LISP_FALSE? LISP_TRUE: LISP_FALSE;
    }
    else 
        return LISP_FALSE;
}

Expr::Ptr eval_or(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 2, "or");
    auto e1 = args->expr->eval(env);
    if(e1 == LISP_FALSE) {
        auto e2 = std::static_pointer_cast<List>(args->next)->expr->eval(env);
        return e2 != LISP_FALSE? LISP_TRUE: LISP_FALSE;
    }
    else 
        return LISP_TRUE;
}

Expr::Ptr eval_not(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 1, "not");
    auto e = args->expr->eval(env);
    return e == LISP_FALSE? LISP_TRUE: LISP_FALSE;
}

Expr::Ptr eval_nullq(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 1, "null?");
    auto val = eval_arguments(args, env);
    return List::can_iterate(std::static_pointer_cast<List>(val->expr))?
           LISP_FALSE: LISP_TRUE;
}

Expr::Ptr eval_eqq(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 2, "eq?");
    auto arg_val = eval_arguments(args, env);
    auto e1 = arg_val->expr, e2 = std::static_pointer_cast<List>(arg_val->next)->expr;
    return e1 == e2? LISP_TRUE: LISP_FALSE;
}

Expr::Ptr eval_pairq(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 1 ,"pair?");
    return args->expr->eval(env)->type == Expr::Type::List ? LISP_TRUE: LISP_FALSE;
}

Expr::Ptr eval_numberq(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 1 ,"pair?");
    return args->expr->eval(env)->type == Expr::Type::Number? LISP_TRUE: LISP_FALSE;
}

Expr::Ptr eval_symbolq(List::Ptr args, Environment::Ptr env)
{
    check_arguments(args, 1 ,"pair?");
    return args->expr->eval(env)->type == Expr::Type::Symbol? LISP_TRUE: LISP_FALSE;
}

