#pragma once

#include "ast.h"
#include "fmt/core.h"
#include <iostream>

namespace Parser {

std::vector<std::string> tokenize(const char *c);

struct Range
{
	using Iter = std::vector<std::string>::iterator;
public:
	Range()=default;
	Range(const Iter& start, const Iter& end): first(start), second(end) {}
	//Range& operator=(const Range& r) { first = r.first; second = r.second; return *this;}

	bool eof() const{ return first == second; }
	const auto& cur() const { return *first; }

	bool match(std::initializer_list<std::string> lits) const { return std::equal(first, second, lits.begin(), lits.end()); }
	Range operator+(int i) const { return {first+i, second}; }
	Range& operator+=(int i) { first += i; return *this; }


	Iter first, second;
};

template<class T>
struct is_tuple: std::false_type {};

template<class ...Ts>
struct is_tuple<std::tuple<Ts...>>: std::true_type {};

template<typename RT>
class Result
{
public:
	using value_type = RT;

	Result(): succ_(false) {}
	Result(const std::string& e): succ_(false), err_(e) {}
	Result(RT&& r, const Range& rst):succ_(true), value_(std::move(r)), rest_(rst) {}
	Result(const Range& rst): succ_(false), rest_(rst) {}
	
	template<typename RT2>
	Result(Result<RT2> &rt): succ_(bool(rt)), value_(rt.getValue()), rest_(rt.rest()), err_(rt.getErr()) {}

	template<typename RT2>
	Result(Result<RT2> &&rt): succ_(bool(rt)), value_(std::move(rt.getValue())), rest_(rt.rest()), err_(std::move(rt.getErr())) {}

	template<class F>
	auto mapWith(F&& f) &&
	{
		if constexpr(is_tuple<RT>::value) {
			using RT2 = decltype(std::apply(f, std::move(value_)));
			return succ_? Result<RT2>{std::apply(f, std::move(value_)), rest_}: Result<RT2>{std::move(err_)};
		}else {
			using RT2 = decltype(f(std::move(value_)));
			return succ_? Result<RT2>{f(std::move(value_)), rest_}: Result<RT2>{std::move(err_)};
		}
	}

	operator bool() { return succ_; }

	const RT& getValue() const { return value_; }
	RT& getValue() { return value_; }
	const Range& rest() const { return rest_; }
	void setRest(const Range& rg) { rest_ = rg; }

	void setErr(const std::string& err) { succ_ = false; err_ = err; }
	std::string& getErr() {  return err_; }

	void setStatus(bool b) { succ_ = b; }

private:
	bool succ_;
	RT value_;
	Range rest_;
	std::string err_;
};

template<>
struct Result<void>
{
	using value_type = void;

	Result(): succ_(false) {}
	Result(bool b, const Range& r): succ_(b), rest_(r) {}
	Result(const std::string& e): succ_(false), err_(e) {}
	
	template<class F>
	auto mapWith(F&& f) &&
	{
		using RT2 = decltype(f());
		if constexpr( std::is_same_v<void, RT2>) {
			if(succ_) {
				f();
				return Result<RT2>{true, rest_};
			}else {
				return Result<RT2>{std::move(err_)};
			}
		}else {
			return succ_? Result<RT2>{f(), rest_}: Result<RT2>{std::move(err_)};
		}
	}

	operator bool() { return succ_; }

	const Range& rest() const { return rest_; }
	void setRest(const Range& rg) { rest_ = rg; }

	void setErr(const std::string& err) { succ_ = false; err_ = err; }
	std::string& getErr() {  return err_; }

	void setStatus(bool b) { succ_ = b; }

	bool succ_;
	Range rest_;
	std::string err_;
};


template<class P, class F,
	class dummy = std::enable_if_t<std::is_invocable<P, const Range&>::value, void>>
auto operator>>(P&& p, F&& f)
{
	return [p=std::forward<P>(p), f=std::forward<F>(f)](const Range& rg)
	{
		return p(rg).mapWith(f);
	};
}

class Datum
{
public:
	enum class Type { Number, Boolean, Symbol, Pair, Nil} type_;

	using Ptr = std::shared_ptr<Datum>;

	Datum(Type t): type_(t) {}
	virtual ~Datum()=0;
};

inline Datum::~Datum() {}

struct DatumNil: public Datum
{
public:
	static auto getInstance() {
		static std::shared_ptr<DatumNil> nil{new DatumNil};
		return nil;
	}
	~DatumNil(){}
private:
	DatumNil():Datum(Datum::Type::Nil) {}
};

struct DatumNum: public Datum
{
	DatumNum(int64_t v): Datum(Datum::Type::Number), value_(v) {}
	~DatumNum(){}

	int64_t value_;
};

struct DatumBool: public Datum
{
	DatumBool(bool v): Datum(Datum::Type::Boolean), value_(v) {}
	~DatumBool(){}

	bool value_;
};

struct DatumSym: public Datum
{
	DatumSym(const std::string& s):Datum(Datum::Type::Symbol), value_(s) {};
	~DatumSym(){}

	std::string value_;
};

struct DatumPair: public Datum
{
	DatumPair(const Datum::Ptr& car = nullptr, const Datum::Ptr& cdr=nullptr): 
		Datum(Datum::Type::Pair), car_(car), cdr_(cdr) {}
	~DatumPair(){}

	Datum::Ptr car_, cdr_;
};



Result<Datum::Ptr> parseDatum(const Range&);

Result<Expr::Ptr> parse(const Range&);
Result<std::unique_ptr<NumberE>> parseNumber(const Range&);
Result<std::unique_ptr<Var>> parseVar(const Range&);
Result<std::unique_ptr<Quote>> parseQuote(const Range&);
Result<std::unique_ptr<Define>>  parseDef(const Range&);
Result<std::unique_ptr<SetBang>>  parseSetBang(const Range&);
Result<std::unique_ptr<Begin>>  parseBegin(const Range&);
Result<std::unique_ptr<If>>  parseIf(const Range&);
Result<std::unique_ptr<Let>>  parseLet(const Range&);
Result<std::unique_ptr<Lambda>>  parseLambda(const Range&);
Result<std::unique_ptr<Apply>>  parseApply(const Range&);

inline auto Lit(const char* c)
{
	return [c](const Range& rg)
	{
		if(!rg.eof() && rg.cur() == c) {
			//std::cerr << "[Lit]" << "matched " << c << std::endl;
			return Result<void>{true, rg+1};
		}else {
			return Result<void>(fmt::format("expect \"{}\", got \"{}\"", c, rg.cur()));
		}
	};
}

template<class RT>
struct Choose 
{

	template<class P0, class ...Parser>
	static auto OneOf(P0& p0, Parser& ...pn)
	{
		return [&](const Range& rg)
		{
			return OneOf(rg, p0, pn...);
		};
	}

	template<class P0, class ...Parser>
	static Result<RT> OneOf(const Range& rg, P0& p0, Parser& ...pn)
	{
		auto r0 = p0(rg);
		if(r0) {
			return r0;
		}else {
			return OneOf(rg, pn...);
		}
	}

	template<class P0>
	static Result<RT> OneOf(const Range& rg, P0& p0) 
	{ 
		return p0(rg); 
	}
};

template<int N>
struct AllImp 
{
	template<class Res, class P0, class... Pn>
	static void match(Res& res, P0& p0, Pn& ...pn)
	{
		auto r0 = p0(res.rest());
		using RT0 = typename decltype(r0)::value_type;
		if(r0) {
			res.setRest( r0.rest() );
			if constexpr( !std::is_same_v<RT0, void> ) {
				std::get<N>(res.getValue()) = std::move(r0.getValue());
				AllImp<N+1>::match(res, pn...);
			}else {
				AllImp<N>::match(res, pn...);
			}
		}else {
			//std::cerr << "[AllImp]Mismatch at " << N << std::endl;
			res.setErr(std::move(r0.getErr()));
		}
	}

	template<class Res>
	static void match(Res& res) 
	{ 
		//std::cerr << "[AllImp]matched all!" << std::endl; 
		res.setStatus(true);
	}
};


template<class T1, class T2>
struct ConsTup { using type = std::tuple<T1, T2>; };

template<class T1>
struct ConsTup<T1, void> { using type = std::tuple<T1>; };

template<class T0, class ...Tn>
struct ConsTup<T0, std::tuple<Tn...>> { using type = std::tuple<T0, Tn...>; };

template<class T0, class ...Tn>
struct TupAcc { using type = typename ConsTup<T0, typename TupAcc<Tn...>::type>::type; };

template<class ...Tn>
struct TupAcc<void, Tn...> { using type = typename TupAcc<Tn...>::type; };

template<class T>
struct TupAcc<T> { using type = std::tuple<T>; };

template<>
struct TupAcc<void> { using type = void; };

static_assert(std::is_same_v<typename TupAcc<void, void, int, void, char, void>::type, std::tuple<int,char>>);
static_assert(std::is_same_v<typename TupAcc<void, void, Var, Expr::Ptr>::type, std::tuple<Var, Expr::Ptr>>);
static_assert(std::is_same_v<typename TupAcc<int>::type, std::tuple<int>>);

template<typename T>
using parse_result_t = typename std::invoke_result_t<T, const Range&>::value_type;

static_assert(std::is_same_v<parse_result_t<decltype(parse)>, Expr::Ptr>);
static_assert(std::is_same_v<parse_result_t<decltype(parseVar)>, std::unique_ptr<Var>>);

template<class P0, class P1, class ...Pn>
auto All(P0& p0, P1& p1, Pn& ...pn)
{
	using Tup = typename TupAcc<parse_result_t<P0>, parse_result_t<P1>, parse_result_t<Pn>...>::type;
	return [&](const Range& rg) //caution!: capture by reference
	{
		Result<Tup> res;
		res.setRest(rg);
		AllImp<0>::match(res, p0, p1, pn...);
		return res;
	};
}

template<class P>
auto Many(P& p) //one or more
{
	return [&](const Range& rg) 
	{
		using RT0 = typename decltype(p(rg))::value_type;
		std::vector<RT0> rs;

		auto it = rg;
		while(true) 
		{
			auto r = p(it);
			if(!r) break;

			it = r.rest();
			rs.emplace_back(std::move(r.getValue()));
		}
		return rs.empty()? Result<std::vector<RT0>>{}: Result<std::vector<RT0>>{std::move(rs), it};
	};
}

template<class P>
auto MaybeMany(P& p) //zero or more
{
	return [&](const Range& rg) 
	{
		using RT0 = typename decltype(p(rg))::value_type;
		std::vector<RT0> rs;

		auto it = rg;
		while(true) 
		{
			auto r = p(it);
			if(!r) break;

			it = r.rest();
			rs.emplace_back(std::move(r.getValue()));
		}
		return Result<std::vector<RT0>>{std::move(rs), it};
	};
}

template<class P>
auto Maybe(P& p)
{
	return [&](const Range& rg)
	{
		using RT0 = typename decltype(p(rg))::value_type;
		std::vector<RT0> rs;

		auto r = p(rg);
		auto it = rg;
		if(r) {
			rs.emplace_back(std::move(r.getValue()));
			it = r.rest();
		}
		return Result<std::vector<RT0>>{std::move(rs), it};
	};
}

}//namespace Parser
