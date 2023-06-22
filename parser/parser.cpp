#include "parser.h"
#include <charconv>

namespace Parser {

using namespace std;

namespace Common
{
	static auto lP = Lit("("), rP = Lit(")"), Quo = Lit("'"), Dot = Lit(".");
	static auto manyVar = Many(parseVar);
	static auto maybeManyVar = MaybeMany(parseVar);
}


Result<vector<Expr::Ptr>> parseProgram(const Range& rg0) 
{
    vector<Expr::Ptr> es;
    auto rg = rg0;
    while (!rg.eof()) {
        auto res = parseExp(rg);
        if (!res)
            return {res.getErr()};

        es.emplace_back(std::move(res.getValue()));
        rg = res.rest();
    }
    return {std::move(es), rg};
}

Result<Expr::Ptr> parseExp(const Range& rg)
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
		return  Choose<Expr::Ptr>::OneOf(rg, 
                        parseDef, 
                        parseQuote, 
                        parseLet, 
                        parseLetRec, 
                        parseLambda, 
                        parseIf, 
                        parseSetBang,
                        parseBegin,
                        parseApply);
	}
	else {
		return parseVar(rg);
	}
}

Result<std::unique_ptr<NumberE>> parseNumber(const Range& rg)
{
	if(rg.eof()) return {};

    NumberE::Type   val;
    auto cur        = rg.cur();
    auto s          = cur.data(),   e = cur.data()+cur.size();
    auto [pos, err] = std::from_chars(s, e, val);

    if(pos == e) {
        //cout << "[parseNumber]" << "parsed num: " << val->value_ << endl;
        return {std::make_unique<NumberE>(val), rg+1};
    } else {
        //cout << "[parseNumber]" << "not a valid number: pos=" << pos << endl;
        return {"not a valid number"};
    }
}

Result<std::unique_ptr<Var>> parseVar(const Range& rg)
{
	if(rg.eof()) return {};


	const auto& cur = rg.cur();
	if(!cur.empty()) {
		switch(cur[0]) {
			case '(':
			case ')':
			case '#':
			case '\'':
			case '.':
				return {};
			default:
				//cout << "[parseVar]" << "matched var: " << rg.cur() << endl;
				return {make_unique<Var>(cur), rg+1};

		}
	}
	return {};
}

Result<std::unique_ptr<Quote>> parseQuote(const Range& rg)
{
	static auto quote = Lit("quote");
	static auto quo1 = All(Common::Quo, parseDatum) >> 
		[](Datum::Ptr&& datum) { return make_unique<Quote>(std::move(datum)); };
	static auto quo2 = All(Common::lP, quote, parseDatum, Common::rP) >>
		[](Datum::Ptr&& datum) { return make_unique<Quote>(std::move(datum)); };

	static auto P = Choose<unique_ptr<Quote>>::OneOf(quo1, quo2);
	return P(rg);
}

Result<std::unique_ptr<Define>>  parseDef(const Range& rg)
{
	static auto defKw = Lit("define");
    // (define <id> <expr>)
	static auto def1 = All(Common::lP, defKw, parseVar, parseExp, Common::rP) >> 
		[](unique_ptr<Var>&& v, Expr::Ptr&& body) 
		{ 
			return make_unique<Define>(std::move(*v), std::move(body)); 
		};

    // (define (<id>...) <expr>)
	static auto def2 = All(Common::lP, defKw, Common::lP, Common::manyVar, Common::rP, parseExp, Common::rP) >>
		[](vector<unique_ptr<Var>>&& vars, Expr::Ptr&& body)
		{
			vector<Var> params;
			for(auto it = vars.begin()+1; it != vars.end(); ++it)
				params.emplace_back(std::move(**it));
			return make_unique<Define>(std::move(*vars[0]), std::make_unique<Lambda>(params, std::move(body)));
		};

	static auto P = Choose<unique_ptr<Define>>::OneOf(def1, def2);
	return P(rg);
}

Result<std::unique_ptr<SetBang>>  parseSetBang(const Range& rg)
{
	static auto setKw = Lit("set!");
	static auto P = All(Common::lP, setKw, parseVar, parseExp, Common::rP) >> 
	[](unique_ptr<Var>&& var, unique_ptr<Expr>&& expr)
	{
		return make_unique<SetBang>(std::move(*var), std::move(expr));
	};

	return P(rg);
}

Result<std::unique_ptr<Begin>>  parseBegin(const Range& rg)
{
	static auto beginKw = Lit("begin");
	static auto manyExp = MaybeMany(parseExp);
	static auto P = All(Common::lP, beginKw, manyExp, Common::rP) >>
		[](vector<unique_ptr<Expr>> &&es)
		{
			return make_unique<Begin>(std::move(es));
		};

	return P(rg);
}

Result<std::unique_ptr<If>>  parseIf(const Range& rg)
{
	static auto ifKw = Lit("if");
	static auto p = All(Common::lP, ifKw, parseExp, parseExp, parseExp, Common::rP) >>
		[](Expr::Ptr&& pred, Expr::Ptr&& thn, Expr::Ptr&& els)
		{
			return std::make_unique<If>(std::move(pred), std::move(thn), std::move(els));
		};
	return p(rg);
}

Result<std::unique_ptr<Lambda>>  parseLambda(const Range& rg)
{

	static auto lambda = Lit("lambda");
	static auto P = All(Common::lP, lambda, Common::lP, Common::maybeManyVar, Common::rP, parseExp, Common::rP) >>
		[](vector<unique_ptr<Var>>&& vars, Expr::Ptr&& body)
		{
			vector<Var> params;
			for(auto& v: vars) params.emplace_back(std::move(*v));
			return make_unique<Lambda>(std::move(params), std::move(body));
		};
	return P(rg);
}


template<class Ast>
Result<std::unique_ptr<Ast>>  parseLetLike(const Range& rg, const char* head)
{
	static auto let = Lit(head);
	static auto bindPair = All(Common::lP, parseVar, parseExp, Common::rP);
	static auto binds = Many(bindPair);

	static auto P = All(Common::lP, let, Common::lP, binds, Common::rP, parseExp, Common::rP) >> 
		[](vector<tuple<unique_ptr<Var>,Expr::Ptr>>&& binds, Expr::Ptr&& b)
		{
			vector<pair<Var, Expr::Ptr>> kvs;
			for(auto& kv: binds) kvs.emplace_back(std::move(*get<0>(kv)), std::move(get<1>(kv)));

			return make_unique<Ast>(std::move(kvs), std::move(b));
		};
	return P(rg);
}

Result<std::unique_ptr<Let>>  parseLet(const Range& rg) {
    return parseLetLike<Let>(rg, "let");
}

Result<std::unique_ptr<LetRec>>  parseLetRec(const Range& rg) {
    return parseLetLike<LetRec>(rg, "letrec");
}

Result<std::unique_ptr<Apply>>  parseApply(const Range& rg)
{
	static auto manyExp = Many(parseExp);
	static auto P = All(Common::lP, manyExp, Common::rP) >>
		[](vector<Expr::Ptr>&& es)
		{
			auto rator = std::move(es[0]);
			es.erase(es.begin());
			return make_unique<Apply>(std::move(rator), std::move(es));
		};

	return P(rg);
}


//parse the program text as data
Result<Datum::Ptr> parseDatum(const Range& rg)
{
	static auto num = parseNumber >> [](unique_ptr<NumberE>&& n) { return make_shared<DatumNum>(n->value_); };
	static auto sym = parseVar >> [](unique_ptr<Var>&& v) { return make_shared<DatumSym>(std::move(v->v_)); };
	static auto datums = Many(parseDatum);

	static auto tail = All(Common::Dot, parseDatum) >> [](Datum::Ptr&& d) { return d; };
	static auto maybeTail = Maybe(tail);
	static auto cons = All(Common::lP, datums, maybeTail, Common::rP) >>
		[](vector<Datum::Ptr>&& vals, vector<Datum::Ptr>&& tail) 
		{ 
			DatumPair head;
			auto it = &head;
			for(auto& v: vals) {
				it->cdr_ = make_shared<DatumPair>(v);
				it = static_cast<DatumPair*>(it->cdr_.get());
			}

			if(tail.empty()) {
				it->cdr_ = DatumNil::getInstance();
			}else{
				it->cdr_ = std::move(tail.back());
			}

			return head.cdr_;
		};
	static auto nil = All(Common::lP, Common::rP) >> []() { return DatumNil::getInstance(); };
	static auto P = Choose<Datum::Ptr>::OneOf(num, sym, cons, nil);
	return P(rg);
}

}//namespace Parser
