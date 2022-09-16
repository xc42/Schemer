#include "parser.h"
#include "value.h"

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

Expr::Ptr parse(std::vector<std::string>::iterator &start,
                std::vector<std::string>::iterator &end)
{
    if(start == end)  return nullptr;

    const auto &str = *start;
    //numbers
    if(std::isdigit(str[0]) || (str[0] == '-' && std::isdigit(str[1]))) {
		return parseNumber(start, end);
    }
	//bool literal
	if(str == "#t") {
		return make_unique<BooleanE>(true);
		++start;
	}
	else if( str == "#f") {
		return make_unique<BooleanE>(false);
		++start;
	}
    //quote
    else if(*start == "'") {
        ++start;
		return parseQuote(start, end);
    }
    //sexp
    else if(*start == "(") {
		if(start + 1 == end) throw ParseException("unclosed left paren");

		const auto& next = *(start+1);
        if(next == "define") {
			return parseDef(start, end);
        }else if(next == "if") {
			return parseIf(start, end);
        }else if(next == "lambda") {
			return parseLambda(start, end);
		}else if(next == "let") {
			return parseLet(start ,end);
        }else { //application
			return parseApply(start, end);
        }
    }
    //identifier or var
	else {
		return parseVar(start, end);
	}
}

std::unique_ptr<Var> parseVar(Iter &start, Iter &end) {
	if(start == end)
		throw ParseException("expect a token as var");

	return std::make_unique<Var>(*start++); 
}

std::unique_ptr<Define>  parseDef(Iter &start, Iter &end)
{
	start += 2;
	unique_ptr<Define> def;
	if(*start == "(") {
		++start;
		auto head = parseVar(start, end);
		vector<Var> rest;
		while(start != end && *start != ")") {
			auto v = parseVar(start, end);
			rest.emplace_back(std::move(*v));
		}
		def = make_unique<Define>(std::move(*head), make_unique<Lambda>(rest, parse(start, end)));
	}else {
		auto head = parseVar(start, end);
		def = make_unique<Define>(std::move(*head), parse(start,end));
	}
	
	if(start == end || *start != ")")
		throw ParseException("unclosed left paren in define");
	++start;
	return def;
}

std::unique_ptr<If> parseIf(Iter &start, Iter &end)
{
	start += 2;
	auto pred = parse(start, end);
	auto cons = parse(start, end);
	auto alter = parse(start, end);
	if(start == end || *start != ")")
		throw ParseException("unclosed left paren in if");

	++start;
	return std::make_unique<If>(std::move(pred), std::move(cons), std::move(alter));
}

std::unique_ptr<Lambda>  parseLambda(Iter &start, Iter &end)
{
	start += 2;

	if(start == end || *start != "(")
		throw ParseException("expect param list in lambda");

	++start;
	Lambda::ParamsType params;
	while(start != end && *start != ")") {
		auto v = parseVar(start, end);
		params.emplace_back(std::move(*v));
	}

	if(start == end)
		throw ParseException("unclosed left paren of lambda param list");

	++start; //skip ")"
	auto body = parse(start, end);

	if(start == end)
		throw ParseException("unclosed left paren of lambda expression");
	++start;
	return std::make_unique<Lambda>(std::move(params), std::move(body));
}

std::unique_ptr<Let>  parseLet(Iter &start, Iter &end)
{
	start += 2;
	
	static auto parseBind = [](Iter &start, Iter &end)
	{
		if(start != end && *start == "(") ++start;
		else throw ParseException("expect left paren in `let` binding");

		auto v = parseVar(start, end);
		auto e = parse(start, end);
		if(start == end || *start != ")") 
			throw ParseException("unclosed right paren in `let` binding");
		++start;
		return make_pair(std::move(*v), std::move(e));
	};

	if(start == end || *start != "(")
		throw ParseException("expect bingding list in `let`");

	++start;
	Let::Binding binds;
	while(start != end && *start != ")") {
		binds.emplace_back(parseBind(start, end));
	}
	if(start == end)
		throw ParseException("unclosed left paren in `let` binding");

	++start;
	auto let = make_unique<Let>(std::move(binds), parse(start, end));
	if(start == end || *start != ")")
		throw ParseException("unclosed left paren in `let`");
	++start;
	return let;
}

std::unique_ptr<Apply>  parseApply(Iter &start, Iter &end)
{
	++start;
	auto rator = parse(start, end);
	Apply::Operands rands;
	while(start != end && *start != ")") {
		rands.emplace_back(parse(start, end));
	}
	if(start == end)
		throw ParseException("unclosed left paren in `apply`");

	++start;
	return std::make_unique<Apply>(std::move(rator), std::move(rands));
}


//parse the program text as data
unique_ptr<Value> parseDatum(Iter &start, Iter &end) 
{
    if(start == end)  return nullptr;

    const auto& str = *start;
    if(std::isdigit(str[0]) || (str[0] == '-' && std::isdigit(str[1]))) {
        return std::make_unique<Number>(std::stoll(*start++)); //
    }
    else if(*start == "(") {
        ++start;
		if(start != end && *start == ")") {
			return make_unique<Nil>();
		}

		auto res = make_unique<Cons>();
		res->car_ = parseDatum(start, end);
		Cons* it = res.get();
        while(start != end && *start != ")") {
			if(*start == ".") { //a cons cell
				++start;
				it->cdr_ = parseDatum(start, end);
				if(start != end && *start == ")") {
					++start;
					return res;
				}else {
					throw ParseException("ill formed cons cell");
				}
			}else {
				auto new_cell = std::make_shared<Cons>(parseDatum(start, end));
				it->cdr_ = new_cell;
				it = new_cell.get();
			}
        }

		if(start != end &&  *start == ")") {
			++start;
			return res;
		}else {
			throw ParseException("unclosed left paren in datum");
		}
    }
    else {
        return std::make_unique<Symbol>(*start++);
    }
}
