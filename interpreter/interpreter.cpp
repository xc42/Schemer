#include "interpreter.h"
#include "parser.h"

namespace Interp {

using namespace std;
using namespace Parser;


Value::Ptr convertDatum(const Datum& dat)
{
	Value::Ptr res;
	switch(dat.type_)
	{
		case Datum::Type::Number:
		{
			res = std::make_shared<Number>(static_cast<const DatumNum&>(dat).value_);
			break;
		}
		case Datum::Type::Boolean:
		{
			res = std::make_shared<Boolean>(static_cast<const DatumBool&>(dat).value_);
			break;
		}
		case Datum::Type::Nil:
		{
			res = Nil::getInstance();
			break;
		}
		case Datum::Type::Symbol:
		{
			res = std::make_shared<Symbol>(static_cast<const DatumSym&>(dat).value_);
			break;
		}
		case Datum::Type::Pair:
		{
			const auto& cons = static_cast<const DatumPair&>(dat);
			res = make_shared<Cons>(convertDatum(*cons.car_), convertDatum(*cons.cdr_));
		}
	}
	return res;
}

void Evaluator::forQuote(const Quote& quo)
{
	static unordered_map<const Quote*, Value::Ptr> datums;

	auto it = datums.find(&quo);
	if(it != datums.end()) {
		result_ = it->second;
		return;
	}

	const auto& dat = *quo.datum_;
	result_ = convertDatum(dat);
	datums.insert({&quo, result_});
}

void Evaluator::forDefine(const Define &def) {
    def.body_->accept(*this);
    env_->bind(def.name_.v_, result_);
	result_ = Void::getInstance();
}

void Evaluator::forSetBang(const SetBang& setBang) {
    
}

void Evaluator::forBegin(const Begin& bgn) {
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
	auto newEnv = Environment::extend(env_);
	for(const auto& kv: let.binds_) {
		kv.second->accept(*this);
        newEnv->bind(kv.first.v_, result_);
	}

    env_ = std::move(newEnv);
	let.body_->accept(*this);
}

void Evaluator::forLetRec(const LetRec& letrec)
{
	for(const auto& kv: letrec.binds_) {
		env_->bind(kv.first.v_, nullptr);
	}


    for (const auto& kv: letrec.binds_) {
        kv.second->accept(*this);
        env_->find(kv.first.v_) = std::move(result_);
    }

	letrec.body_->accept(*this);
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
		const auto& params = *lam.params_;

        env_ = Environment::extend(env_);
		for(int i = 0; i < params.size(); ++i) {
			env_->bind(params[i].v_, std::move(rands[i]));
        }

		lam.body_->accept(*this);
	}else {
		auto proc = static_pointer_cast<Procedure>(rator);
		result_ = proc->func_(rands);
	}
}

void ValuePrinter::forCons(const Cons& cc) {
	os_ << "'(";
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

EnvironmentPtr getInitialTopEnv()
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
} //namespace Interp
