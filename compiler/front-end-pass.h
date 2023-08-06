#pragma once

#include "ast.h"
#include "parser.h"
#include <unordered_set>
#include <vector>
#include <list>
//#include <iostream>

namespace FrontEndPass
{

/*
template<class FoldFn>
struct FoldExpr : public VisitorE 
{
    virtual void forNumber(const NumberE&){}
    virtual void forBoolean(const BooleanE&){}
    virtual void forVar(const Var& v){  }
    virtual void forQuote(const Quote&){}
    virtual void forDefine(const Define&){}
    virtual void forSetBang(const SetBang&){}
    virtual void forBegin(const Begin&){}
    virtual void forIf(const If&){}
	virtual void forLet(const Let&){}
    virtual void forLambda(const Lambda&){}
    virtual void forApply(const Apply&){}

	FoldFn fn;
};

*/


//store some info/data through some passes
struct PassContext
{
	bool isAssigned(const std::string& v) { return assignedVars.count(v); }

	std::unordered_set<std::string> assignedVars; //result from CollectAssign
	std::unordered_set<std::string> freeVars; //result from  FreeVariable
};

class Program
{
public:
	using Pair = std::pair<Define, PassContext>;
	Program(std::vector<std::unique_ptr<Define>> &&defs) { 
		for(auto& def: defs) {
			defs_.emplace_back(std::move(*def), PassContext{});
		}
	}

	auto begin() { return defs_.begin(); }
	auto end() { return defs_.end(); }

private:
	std::list<Pair> defs_;
};

class CollectAssign: public ExprMapper
{
public:
    void forSetBang(const SetBang& setBang) override { 
		auto res = assigned.insert(setBang.v_); 
		//if(res.second) {
		//	std::cerr << "add assigned: " << setBang.v_.v_ << std::endl;
		//}
	}

	static void run(Program& prog) {
		size_t i = 0;
		for(auto& def: prog) {
			CollectAssign collector;
			def.first.body_->accept(collector);
			def.second.assignedVars = std::move(collector.assigned);
		}
	}

private:
	std::unordered_set<std::string> assigned;
};


//functions 
using PassFunc = void(*)(Program&);

Parser::Result<std::unique_ptr<Program>> parseProg(const Parser::Range& rg);


void  runAllPass(Program& p);
std::string gensym(const std::string& s);

}//namespace FrontEndPass
