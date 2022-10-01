#lang racket

(struct Expr ())
(struct Int Expr (value)) 
(struct Bool Expr (value)) 
(struct Var Expr (name)) 
(struct Let Expr (var rhs body)) 
(struct If Expr (cnd thn els)) 
(struct Lambda Expr (param* body))
(struct Apply Expr (fun arg*))
(struct Begin Expr (es e));
(struct SetBang Expr (var e))

(struct Def (name info body))
(struct Program (def* body))

(define (parse-exp e)
  (match e
	[(? symbol?) (Var e)]
	[(? fixnum?) (Int e)]
	[(? boolean?) (Bool e)]
	[`(let ([,xs ,rhs] ...) ,body)
	;;TODO 这里其实是当作let*的语义了,暂时没决定好怎么处理多个绑定的let
	  (for/foldr ([acc body])
				 ([x xs]
				  [r rhs])
				 (Let x (parse-exp r) acc))]
	[`(if ,cnd ,thn ,els) (If (parse-exp cnd) (parse-exp thn) (parse-exp els))]
	[`(cond (,pred ,body) ..1 (else ,els))
	  (for/foldr ([acc (parse-exp els)])
				 ([p pred]
				  [b body])
				 (If (parse-exp p) (parse-exp b) acc))]
	[`(lambda ,ps ,body) 
	  (Lambda ps (parse-exp body))]
    [`(set! ,x ,rhs)
     (SetBang x (parse-exp rhs))]
    [`(begin ,es ... ,e)
     (Begin (for/list ([e es]) (parse-exp e)) (parse-exp e))]
	))

(define (fmap-expr f expr)
  (match expr
	[(If e1 e2 e3) (If (f e1) (f e2) (f e3))]
	[(Apply func args) (Apply (f func) (map f args))]
	[(Let v e body) (Let v (f e) (f body))]
	[(Lambda ps body) (Lambda ps (f body))]
	[(SetBang v e) (SetBang v (f e))]
	[(Begin es e) (Begin (map f es) (f e))]
	))



(define (uniqify p) '())

(define (hoist-complex-datum p) '())

(define (assignment-conversion p) '())


