#include "lisp.h"

Value::Ptr cons(Appliable::Arg& args) {
    check_args_exact(args, 2);
    return std::make_shared<Cons>(args[0], args[1]);
}


