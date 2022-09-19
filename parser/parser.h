#pragma once

#include "ast.h"

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

template<typename RT>
struct Result
{
	using value_type = RT;

	Result(): succ_(false) {}
	Result(const std::string& e): succ_(false), err_(e) {}
	Result(RT&& r, const Range& rst):succ_(true), value_(std::move(r)), rest_(rst) {}
	Result(const Range& rst): succ_(false), rest_(rst) {}
	
	template<typename RT2>
	Result(Result<RT2> &rt): succ_(rt.succ_), value_(rt.value_), rest_(rt.rest_), err_(rt.err_) {}

	template<typename RT2>
	Result(Result<RT2> &&rt): succ_(rt.succ_), value_(std::move(rt.value_)), rest_(std::move(rt.rest_)), err_(std::move(rt.err_)) {}

	/*
	auto& operator=(Result<RT>&& rt) {
		succ_ = rt.succ_;
		value_ = std::move(rt.value_);
		rest_ = std::move(rt.rest_);
		err_ = std::move(rt.err_);
		return *this;
	} 
	*/

	operator bool() { return succ_; }

	bool succ_;
	RT value_;
	Range rest_;
	std::string err_;
};

template<class P, class F>
auto operator>>(P&& p, F&& f)
{
	return [&](const Range& rg)
	{
		auto r = p(rg);
		using RT = decltype(std::apply(f, std::move(r.value_)));
		if(r) {
			return Result<RT>{std::apply(f, std::move(r.value_)), r.rest_};
		}else {
			return Result<RT>{std::move(r.err_)};
		}
	};
}

template<>
struct Result<void>
{
	using value_type = void;

	Result(): succ_(false) {}
	Result(bool b, const Range& r): succ_(b), rest_(r) {}
	Result(const std::string& e): succ_(false), err_(e) {}
	
	operator bool() { return succ_; }

	bool succ_;
	Range rest_;
	std::string err_;
};

Result<std::shared_ptr<Value>> parseDatum(const Range&);


Result<Expr::Ptr> parse(const Range&);
Result<std::unique_ptr<NumberE>> parseNumber(const Range&);
Result<std::unique_ptr<Var>> parseVar(const Range&);
Result<std::unique_ptr<Quote>> parseQuote(const Range&);
Result<std::unique_ptr<Define>>  parseDef(const Range&);
Result<std::unique_ptr<If>>  parseIf(const Range&);
Result<std::unique_ptr<Let>>  parseLet(const Range&);
Result<std::unique_ptr<Lambda>>  parseLambda(const Range&);
Result<std::unique_ptr<Apply>>  parseApply(const Range&);

inline auto Lit(const char* c)
{
	return [c](const Range& rg)
	{
		if(rg.eof() && rg.cur() == c) {
			return Result<void>{true, rg+1};
		}else {
			return Result<void>(false, rg);
		}
	};
}

template<class RT>
struct Or 
{
	template<class P0, class ...Parser>
	static auto OneOf(P0&& p0, Parser&& ...pn)
	{
		return [&](const Range& rg) -> Result<RT>
		{
			auto r0 = p0(rg);
			if(r0) {
				return r0;
			}else {
				return OneOf(std::forward<Parser>(pn)...)(rg);
			}
		};
	}

	template<class P0>
	static auto OneOf(P0&& p0) 
	{ 
		return [&](const Range& rg)
		{
			return p0(rg); 
		};
	}
};

template<int N>
struct All 
{
	template<class Res, class P0, class... Pn>
	static void match(Res& res, P0&& p0, Pn&& ...pn)
	{
		auto r0 = p0(res.rest_);
		using RT0 = typename decltype(r0)::value_type;
		if(r0) {
			res.rest_ = r0.rest_;
			if constexpr( !std::is_same_v<RT0, void> ) {
				std::get<N>(res.value_) = std::move(r0.value_);
				All<N+1>::match(res, std::forward<Pn>(pn)...);
			}else {
				All<N>::match(res, std::forward<Pn>(pn)...);
			}
		}else {
			res.err_ = std::move(r0.err_);
		}
	}

	template<class Res>
	static void match(Res& res) { }
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

template<typename T>
using parse_result_t = typename std::invoke_result_t<T, const Range&>::value_type;

static_assert(std::is_same_v<parse_result_t<decltype(parse)>, Expr::Ptr>);
static_assert(std::is_same_v<parse_result_t<decltype(parseVar)>, std::unique_ptr<Var>>);

template<class P0, class P1, class ...Pn>
auto And(P0&& p0, P1&& p1, Pn&& ...pn)
{
	using Tup = typename TupAcc<parse_result_t<P0>, parse_result_t<P1>, parse_result_t<Pn>...>::type;
	return [&](const Range& rg)
	{
		Result<Tup> res;
		res.rest_ = rg;
		All<0>::match(res, std::forward<P0>(p0), std::forward<P1>(p1), std::forward<Pn>(pn)...);
		return res;
	};
}

template<class P>
auto Many(P&& p) //one or more
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

			it = r.rest_;
			rs.emplace_back(std::move(r.value_));
		}
		return rs.empty()? Result<std::vector<RT0>>{}: Result<std::vector<RT0>>{std::move(rs), it};
	};
}

template<class P>
auto MaybeMany(P&& p) //zero or more
{
	return [&](const Range& rg) 
	{
		using RT0 = typename decltype(p(rg))::value_type;
		std::vector<RT0> rv;

		Result<RT0> r;
		r.rest_ = rg;
		do {
			r = p(r.rest_);
			if(r) rv.emplace_back(std::move(r.value_));
		}while(r);

		return Result<std::vector<RT0>>{std::move(rv), r.rest_};
	};
}


}//namespace Parser
