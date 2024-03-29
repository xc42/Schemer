#pragma once

#include <vector>
#include <map>
#include <string>
#include <functional>
#include <memory>
#include <optional>

class VisitorE; //forward declaration of base visitor

//Expr = NumberE| SymbolE| Quote| Define| Let| If| Lambda| Application
struct Expr {
    using Ptr = std::unique_ptr<Expr>;

    enum class Type {Number, Boolean, Var, Quote, Define, SetBang, Begin, Let, If, Lambda, Apply} type_;

    Expr(Type t):type_(t){}
    virtual ~Expr()=default;

    Type getType() const { return type_; }

    virtual void accept(VisitorE& visitor) const = 0;
};

struct NumberE: Expr{
    using Type = long long;
    NumberE(Type v): Expr(Expr::Type::Number), value_(v){};
    void accept(VisitorE &v) const override;

    Type value_;
};

struct BooleanE: Expr {
	BooleanE(bool b): Expr(Expr::Type::Boolean), b_(b) {};
    void accept(VisitorE &v) const override;

	bool b_;
};

struct Var: Expr{
	Var():Expr(Expr::Type::Var) {}
    Var(std::string_view s):Expr(Expr::Type::Var),v_(s){}
    void accept(VisitorE &v) const override;

	operator const std::string&() const { return v_; }
    const std::string v_;    
};

struct Define: Expr {
    Define(const Var& n, Expr::Ptr &&b):
        Expr(Expr::Type::Define), name_(n), body_(std::move(b)){}

    void accept(VisitorE &v) const override;

    Var name_;
    Expr::Ptr body_;
};

struct SetBang: Expr {
	SetBang(const Var& v, Expr::Ptr&& e): Expr(Expr::Type::SetBang), v_(v), e_(std::move(e)){}

    void accept(VisitorE &v) const override;

	Var v_;
	Expr::Ptr e_;
};

struct If: Expr{
    If(Expr::Ptr &&p, Expr::Ptr &&c, Expr::Ptr &&a):
        Expr(Expr::Type::If), pred_(std::move(p)), thn_(std::move(c)), els_(std::move(a)){}

    void accept(VisitorE &v) const override;

    Expr::Ptr pred_, thn_, els_;
};

struct Let: Expr {
	using Binding = std::vector<std::pair<Var, Expr::Ptr>>;

	Let(Binding ve, Expr::Ptr b):Expr(Expr::Type::Let), binds_(std::move(ve)), body_(std::move(b)) {}

	void accept(VisitorE &v) const override;
	Binding binds_;
	Expr::Ptr body_;
};

struct LetRec: public Let {
    using Let::Let;
	void accept(VisitorE &v) const override;
};

struct Lambda: Expr{
	using ParamsType = std::vector<Var>;
	Lambda(ParamsType::const_iterator b, ParamsType::const_iterator e, Expr::Ptr&& body):
        Expr(Expr::Type::Lambda), params_(std::make_shared<ParamsType>(b, e)), body_(std::move(body)){}

    Lambda(const ParamsType& params, Expr::Ptr&& b):Lambda(params.begin(), params.end(), std::move(b)) {}

	auto arity() const { return params_->size(); }

    void accept(VisitorE &v) const override;

	std::shared_ptr<ParamsType> params_;
	std::shared_ptr<Expr> body_;
};

struct Begin: Expr {
	Begin(std::vector<Expr::Ptr>&& es):Expr(Expr::Type::Begin), es_(std::move(es)) {}
    void accept(VisitorE &v) const override;
	std::vector<Expr::Ptr> es_;
};

struct Apply: Expr{
    using Operands = std::vector<Expr::Ptr>;
    Apply(Expr::Ptr &&rator, Operands&& rands):
        Expr(Expr::Type::Apply), operator_(std::move(rator)), operands_(std::move(rands)){}

    void accept(VisitorE &v) const override;

    Expr::Ptr operator_;
    Operands operands_;
};

namespace Parser{
	class Datum;
}

struct Quote: Expr {
    Quote(const std::shared_ptr<Parser::Datum>& v):Expr(Expr::Type::Quote), datum_(v){}
    void accept(VisitorE &v) const override;

	std::shared_ptr<Parser::Datum> datum_;
};


class VisitorE {
public:
    virtual void forNumber(const NumberE&)=0;
    virtual void forBoolean(const BooleanE&)=0;
    virtual void forVar(const Var&)=0;
    virtual void forQuote(const Quote&)=0;
    virtual void forDefine(const Define&)=0;
    virtual void forSetBang(const SetBang&)=0;
    virtual void forBegin(const Begin&)=0;
    virtual void forIf(const If&)=0;
	virtual void forLet(const Let&)=0;
	virtual void forLetRec(const LetRec&)=0;
    virtual void forLambda(const Lambda&)=0;
    virtual void forApply(const Apply&)=0;
    virtual ~VisitorE()=default;
};

inline void NumberE::accept(VisitorE& v) const { v.forNumber(*this); }
inline void BooleanE::accept(VisitorE& v) const { v.forBoolean(*this); }
inline void Var::accept(VisitorE& v) const { v.forVar(*this); }
inline void Quote::accept(VisitorE& v) const { v.forQuote(*this); }
inline void Define::accept(VisitorE& v) const { v.forDefine(*this); }
inline void SetBang::accept(VisitorE& v) const { v.forSetBang(*this); }
inline void Begin::accept(VisitorE& v) const { v.forBegin(*this); }
inline void Let::accept(VisitorE& v) const { v.forLet(*this); }
inline void LetRec::accept(VisitorE& v) const { v.forLetRec(*this); }
inline void If::accept(VisitorE& v) const { v.forIf(*this); }
inline void Lambda::accept(VisitorE& v) const { v.forLambda(*this); }
inline void Apply::accept(VisitorE& v) const { v.forApply(*this); }

class ExprMapper: public VisitorE
{
public:
    virtual void forNumber(const NumberE&) override {}
    virtual void forBoolean(const BooleanE&) override {}
    virtual void forVar(const Var&) override {}
    virtual void forQuote(const Quote&) override {}
    virtual void forDefine(const Define& def) override { def.body_->accept(*this); }
    virtual void forSetBang(const SetBang& setBang) override { 
		setBang.v_.accept(*this);
		setBang.e_->accept(*this); 
	}
    virtual void forBegin(const Begin& bgn) override { for(const auto& e: bgn.es_) e->accept(*this); }
    virtual void forIf(const If& ifExpr) override { 
		ifExpr.pred_->accept(*this);
		ifExpr.thn_->accept(*this);
		ifExpr.els_->accept(*this);
	}
    template<class Let>
    void forLetLike(const Let& let) {
		for(const auto& kv: let.binds_) {
			kv.second->accept(*this);
		}
		let.body_->accept(*this);
    }
	virtual void forLet(const Let& let) override {
        forLetLike(let);
	}
	virtual void forLetRec(const LetRec& letrec) override {
        forLetLike(letrec);
	}

    virtual void forLambda(const Lambda& lam) override { lam.body_->accept(*this); }

    virtual void forApply(const Apply& app) override {
		app.operator_->accept(*this);
		for(const auto& arg: app.operands_) {
			arg->accept(*this);
		}
	}

};
