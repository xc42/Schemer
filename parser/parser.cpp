#include "parser.h"
#include "value.h"

namespace Parser {
using namespace std;

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
			case '.':
				tokens.push_back(std::string(1, *c++));
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


Result<Expr::Ptr> parse(const Range& rg)
{
	if(rg.eof()) return {};

	const auto &peek = rg.cur();
	//numbers
	if(std::isdigit(peek[0]) || (peek[0] == '-' && std::isdigit(peek[1]))) {
		return parseNumber(rg);
	}
	//bool literal
	if(peek == "#t") {
		return Result<unique_ptr<BooleanE>>{make_unique<BooleanE>(true), rg+1};
	}
	else if( peek == "#f") {
		return Result<unique_ptr<BooleanE>>{make_unique<BooleanE>(false), rg+1};
	}
	else if(peek == "'") {
		return parseQuote(rg);
	}
	else if(peek == "(") {
		static auto sexp = Or<Expr::Ptr>::OneOf(parseDef, parseLet, parseLambda, parseIf, parseApply);
		return sexp(rg);
	}
	else {
		return parseVar(rg);
	}
}

Result<std::unique_ptr<NumberE>> parseNumber(const Range& rg)
{
	if(rg.eof()) return {};
	try {
		size_t pos;
		auto val = make_unique<NumberE>(std::stoll(rg.cur(), &pos));
		if(pos == rg.cur().size())
			return {std::move(val), rg+1};
		else 
			return {"not a valid number"};
	}catch(exception& ex) {
		return {"not a valid number"};
	}
}

Result<std::unique_ptr<Var>> parseVar(const Range& rg)
{
	if(rg.eof()) return {};

	return {make_unique<Var>(rg.cur()), rg+1};
}

Result<std::unique_ptr<Quote>> parseQuote(const Range& rg)
{
	return {};
}

Result<std::unique_ptr<Define>>  parseDef(const Range& rg)
{
	//auto def1 = And(rg, Lit("fuck"), Lit("define"));
	static auto lp = Lit("("), rp = Lit(")"), defKw = Lit("define");
	static auto def1 = And(lp, defKw, parseVar, parse, rp) >> 
		[](unique_ptr<Var>&& v, Expr::Ptr&& body) 
		{ 
			return make_unique<Define>(std::move(*v), std::move(body)); 
		};

	static auto def2 = And(lp, defKw, lp, Many(parseVar), rp, parse, rp) >>
		[](vector<unique_ptr<Var>>&& vars, Expr::Ptr&& body)
		{
			vector<Var> params;
			for(auto it = vars.begin()+1; it != vars.end(); ++it)
				params.emplace_back(std::move(**it));
			return make_unique<Define>(std::move(*vars[0]), std::make_unique<Lambda>(params, std::move(body)));
		};
	static auto P = Or<unique_ptr<Define>>::OneOf(def1, def2);
	return P(rg);
}

Result<std::unique_ptr<If>>  parseIf(const Range& rg)
{
	static auto p = And(Lit("("), Lit("if"), parse, parse, parse) >> 
		[](Expr::Ptr&& pred, Expr::Ptr&& thn, Expr::Ptr&& els)
		{
			return std::make_unique<If>(std::move(pred), std::move(thn), std::move(els));
		};
	return p(rg);
}

Result<std::unique_ptr<Lambda>>  parseLambda(const Range& rg)
{

	static auto lp = Lit("("), rp = Lit(")"), lambda = Lit("lambda");
	static auto P = And(lp, lambda, lp, MaybeMany(parseVar), rp, parse, rp) >>
		[](vector<unique_ptr<Var>>&& vars, Expr::Ptr&& body)
		{
			vector<Var> params;
			for(auto& v: vars) params.emplace_back(std::move(*v));
			return make_unique<Lambda>(std::move(params), std::move(body));
		};
	return P(rg);
}

Result<std::unique_ptr<Let>>  parseLet(const Range& rg)
{
	static auto lp = Lit("("), rp = Lit(")"), let = Lit("let");
	static auto bindPair = And(lp, parseVar, parse, rp) >>
		[](unique_ptr<Var>&& v, Expr::Ptr&& e)
		{
			return make_pair<Var, Expr::Ptr>(std::move(*v), std::move(e));
		};
	static auto P = And(lp, let, lp, Many(bindPair), rp, parse, rp) >> 
		[](vector<pair<Var,Expr::Ptr>>&& binds, Expr::Ptr&& b)
		{
			return make_unique<Let>(std::move(binds), std::move(b));
		};
	return P(rg);
}

Result<std::unique_ptr<Apply>>  parseApply(const Range& rg)
{
	static auto P = And(Lit("("), Many(parse), Lit(")")) >>
		[](vector<Expr::Ptr>&& es)
		{
			auto rator = std::move(es[0]);
			es.erase(es.begin());
			return make_unique<Apply>(std::move(rator), std::move(es));
		};

	return P(rg);
}


//parse the program text as data
Result<std::shared_ptr<Value>> parseDatum(const Range&)
{
	return {};
}

}//namespace Parser
