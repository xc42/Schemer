#pragma once

#include "ast.h"
#include "parser.h"
#include <unordered_set>
#include <vector>
#include <list>

namespace FrontEndPass
{
/*
template<class Fn, class Ret>
struct MapExpr: public VisitorE
{
    virtual void forNumber(const NumberE&){}
    virtual void forBoolean(const BooleanE&){}
    virtual void forVar(const Var&){}
    virtual void forQuote(const Quote&){}
    virtual void forDefine(const Define&){}
    virtual void forSetBang(const SetBang&){}
    virtual void forBegin(const Begin&){}
    virtual void forIf(const If&){}
	virtual void forLet(const Let&){}
    virtual void forLambda(const Lambda&){}
    virtual void forApply(const Apply&){}
};

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
	std::unordered_set<const Var*> assignedVars; //result from CollectAssign
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

class CollectAssign: public VisitorE
{
public:
    void forNumber(const NumberE&){}
    void forBoolean(const BooleanE&){}
    void forVar(const Var&){}
    void forQuote(const Quote&){}
    void forDefine(const Define& def) { def.body_->accept(*this); }
    void forSetBang(const SetBang& setBang) { assigned.insert(&(setBang.v_)); }
    void forBegin(const Begin& bgn) { for(const auto& e: bgn.es_) e->accept(*this); }
    void forIf(const If& ifExpr) { 
		ifExpr.pred_->accept(*this);
		ifExpr.thn_->accept(*this);
		ifExpr.els_->accept(*this);
	}
	void forLet(const Let& let){
		for(const auto& kv: let.binds_) {
			kv.second->accept(*this);
		}
		let.body_->accept(*this);
	}

    void forLambda(const Lambda& lam){
		lam.body_->accept(*this);
	}

    void forApply(const Apply& app){
		app.operator_->accept(*this);
		for(const auto& arg: app.operands_) {
			arg->accept(*this);
		}
	}

	void dump(PassContext& ctx) { ctx.assignedVars = std::move(assigned); }

private:
	std::unordered_set<const Var*> assigned;
};

//functions 
using PassFunc = void(*)(Program&);

Parser::Result<std::unique_ptr<Program>> parseProg(const Parser::Range& rg);




void collectAssign(Program& p);

void  runAllPass(Program& p);

}//namespace FrontEndPass
