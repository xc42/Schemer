#include "lisp.h"

namespace builtin{

Value::Ptr cons(Appliable::Arg&);
Value::Ptr car(Appliable::Arg&);
Value::Ptr cdr(Appliable::Arg&);

template<typename ArithOp>
class Arith {
public:
    Arith():op_(ArithOp()){}
    Value::Ptr operator()(const Appliable::Arg& args) {
        check_args_at_least(args, 2);
        auto result = std::make_shared<NumberV>(ValueChecker<NumberV>::get_ref(**args.begin()));
        for(auto i = ++args.begin(); i != args.end(); ++i) {
            result->value_ = op_(result->value_, ValueChecker<NumberV>::get_ref(**i).value_);
        }
        return result;
    }
private:
    ArithOp op_;
};

template<typename LogicOp>
class Comparator {
public:
    Comparator():op_(LogicOp()){}
    Value::Ptr operator()(const Appliable::Arg& args) {
        check_args_exact(args, 2);
        auto lhs = ValueChecker<NumberV>::get_ref(*args[0]);
        auto rhs = ValueChecker<NumberV>::get_ref(*args[1]);
        return std::make_shared<Boolean>(op_(lhs.value_, rhs.value_));
    }
private:
    LogicOp op_;
};

} //namespace builtin
