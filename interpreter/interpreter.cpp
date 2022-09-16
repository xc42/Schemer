#include "interpreter.h"

using namespace std;

Environment::Ptr Environment::extend(const std::vector<std::pair<Var, Value::Ptr>> &binds, const Environment::Ptr& old)
{
		auto newEnv = std::make_shared<Environment>();
		newEnv->outer_ = old;
		for(const auto& kv: binds)
			newEnv->bind(kv.first, kv.second);
		return newEnv;
}

Value::Ptr Environment::operator()(const Var& s)
{
	auto it = bindings_.find(s);
	if( it != bindings_.end()) {
		return it->second;
	} else if(outer_) {
		return (*outer_)(s);
	}else {
		throw std::runtime_error(std::string(s) + " : undefined");
	}
}

Value::Ptr Evaluator::void_ = make_shared<Void>();

void Evaluator::forDefine(const Define &def) {
    def.body_->accept(*this);
    env_->bind(def.name_, result_);
	result_ = void_;
}

void Evaluator::forIf(const If &if_expr) {
    if_expr.pred_->accept(*this);
    if((result_->type_ == Value::Type::Boolean && static_pointer_cast<Boolean>(result_)->value_ == false) ||
			(result_->type_ == Value::Type::Nil)) {
        if_expr.els_->accept(*this);
    }else {
        if_expr.thn_->accept(*this);
    }
}


void Evaluator::forLet(const Let& let)
{
	vector<pair<Var, Value::Ptr>> binds;
	for(const auto& kv: let.binds_) {
		kv.second->accept(*this);
		binds.emplace_back(kv.first, result_);
	}

	env_ = Environment::extend(binds, env_);
	let.body_->accept(*this);
}


void Evaluator::forApply(const Apply &app) {
    app.operator_->accept(*this);
    auto rator = std::move(result_);

	if(rator->type_ != Value::Type::Closure && rator->type_ != Value::Type::Procedure) 
		throw runtime_error(fmt::format("expect a procedure, got {}", typeStr(rator->type_)));

    std::vector<Value::Ptr> rands;
    for(auto &rand: app.operands_) {
        rand->accept(*this);
        rands.emplace_back(result_);
    }

	if(rator->type_ == Value::Type::Closure) {
		const auto& clos = static_pointer_cast<Closure>(rator);
		checkArityExact(clos->arity(), rands.size());
		
		const auto& lam = *clos->lambda_;
		vector<pair<Var, Value::Ptr>> binds;
		const auto& params = *lam.params_;
		for(int i = 0; i < params.size(); ++i) 
			binds.emplace_back(params[i], rands[i]);

		Evaluator newEv( Environment::extend(binds, clos->env_) );
		lam.body_->accept(newEv);
		result_ = newEv.getResult();
	}else {
		auto proc = static_pointer_cast<Procedure>(rator);
		result_ = proc->func_(rands);
	}
}

void ValuePrinter::forCons(const Cons& cc) {
	os_ << "(";
	cc.car_->accept(*this);
	auto it = cc.cdr_.get();
swloop:
	switch(it->type_) {
		case Value::Type::Nil:
			os_ << ")";
			return;
		case Value::Type::Cons:
			while(it->type_ == Value::Type::Cons) {
				const auto cons = static_cast<Cons*>(it);
				os_ << " ";
				cons->car_->accept(*this);
				it = cons->cdr_.get();
			}
			goto swloop;
		default:
			os_ << " . ";
			it->accept(*this);
			os_ << ")";
			return;
	}
}

namespace builtin
{

using Args = vector<Value::Ptr>;

Value::Ptr cons(const Args& args) {
    checkArityExact(args.size(), 2);
    return std::make_unique<Cons>(args[0], args[1]);
}

Value::Ptr car(const Args& args) {
    checkArityExact(args.size(), 1);
	checkValueType(args[0], Value::Type::Cons);
	return static_cast<Cons&>(*args[0]).car_;
}

Value::Ptr cdr(const Args& args) {
    checkArityExact(args.size(), 1);
	checkValueType(args[0], Value::Type::Cons);
	return static_cast<Cons&>(*args[0]).cdr_;
}

Value::Ptr nullq(const Args& args) {
    checkArityExact(args.size(), 1);
	return make_unique<Boolean>(args[0]->type_ == Value::Type::Nil);
}

Value::Ptr eqq(const Args& args) {
    checkArityExact(args.size(), 2);
	const auto& e1 = *args[0], &e2 = *args[1];
	bool eq = e1.type_ == e2.type_ && 
		(e1.type_ == Value::Type::Number ? 
		 static_cast<const Number&>(e1).value_ == static_cast<const Number&>(e2).value_:
		 e1.type_ == Value::Type::Boolean?
		 static_cast<const Boolean&>(e1).value_ == static_cast<const Boolean&>(e2).value_:
		 &e1 == &e2);
	return make_unique<Boolean>(eq);
}

Environment::Ptr getInitialTopEnv()
{
    auto env = std::make_shared<Environment>();

    using namespace builtin;
    //arithmetic functions
    env->bind("+", std::make_shared<Procedure>(Arith<std::plus<Number::Type>>()));
    env->bind("-", std::make_shared<Procedure>(Arith<std::minus<Number::Type>>()));
    env->bind("*", std::make_shared<Procedure>(Arith<std::multiplies<Number::Type>>()));
    env->bind("/", std::make_shared<Procedure>(Arith<std::divides<Number::Type>>()));

    //loagical functions*
    env->bind("=", std::make_shared<Procedure>(Comparator<std::equal_to<Number::Type>>()));
    env->bind("<", std::make_shared<Procedure>(Comparator<std::less<Number::Type>>()));
    env->bind("<=", std::make_shared<Procedure>(Comparator<std::less_equal<Number::Type>>()));
    env->bind(">", std::make_shared<Procedure>(Comparator<std::greater<Number::Type>>()));
    env->bind(">=", std::make_shared<Procedure>(Comparator<std::greater_equal<Number::Type>>()));

    env->bind("cons", std::make_shared<Procedure>(cons));
    env->bind("car", std::make_shared<Procedure>(car));
    env->bind("cdr", std::make_shared<Procedure>(cdr));
    env->bind("null?", std::make_shared<Procedure>(nullq));
    env->bind("eq?", std::make_shared<Procedure>(eqq));

    return env;
}

}//namespace builtin
