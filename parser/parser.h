#pragma once

#include "ast.h"

std::vector<std::string> tokenize(const char *c);

using Iter = std::vector<std::string>::iterator;
Expr::Ptr parse(Iter &start, Iter &end);
std::unique_ptr<Value> parseDatum(Iter &start, Iter &end);

inline std::unique_ptr<NumberE> parseNumber(Iter &start, Iter &end) 
{ 
	return std::make_unique<NumberE>(std::stoll(*start++)); 
}

inline std::unique_ptr<Quote> parseQuote(Iter &start, Iter &end) 
{ 
	return std::make_unique<Quote>(parseDatum(start, end)); 
}

std::unique_ptr<Var> parseVar(Iter &start, Iter &end);
std::unique_ptr<Define>  parseDef(Iter &start, Iter &end);
std::unique_ptr<Let>  parseLet(Iter &start, Iter &end);
std::unique_ptr<Lambda>  parseLambda(Iter &start, Iter &end);
std::unique_ptr<If>  parseIf(Iter &start, Iter &end);
std::unique_ptr<Apply>  parseApply(Iter &start, Iter &end);


class ParseException:public std::exception
{
public:
	ParseException(const char* c) :what_(c){}

	virtual const char* what() const noexcept override { return what_; }
private:
	const char* what_;
};
