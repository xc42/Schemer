#include "builtins.hpp"

namespace builtin {

Value::Ptr cons(const Appliable::Arg& args) {
    check_args_exact(args, 2);
    return std::make_shared<Cons>(args[0], args[1]);
}

Value::Ptr car(const Appliable::Arg& args) {
    check_args_exact(args, 1);
    return ValueChecker<Cons>::get_ptr(**args.begin())->car_;
}

Value::Ptr cdr(const Appliable::Arg& args) {
    check_args_exact(args, 1);
    return ValueChecker<Cons>::get_ptr(**args.begin())->cdr_;
}

Value::Ptr nullq(const Appliable::Arg& args) {
    check_args_exact(args, 1);
    return std::make_shared<Boolean>(*args.begin() == nullptr);
}

Value::Ptr eqq(const Appliable::Arg& args) {
    check_args_exact(args, 2);
    const auto fst = args.cbegin();
    const auto snd = std::next(fst);
    return std::make_shared<Boolean>(ValueEq::eq(fst->get(), snd->get()));
}

}
