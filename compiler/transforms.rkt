#lang racket

(struct Expr ())
(struct Int Expr (value)) 
(struct Bool Expr (value)) 
(struct Var Expr (name)) 
(struct Quote Expr (dat))
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
	[`(quote ,dat) (Quote dat)]
	[_ (error "unknown exp")]
	))

(define (parse-def d)
  (match d
	[`(define ,name ,body) (Def name '() body)]
	[`(define `(,name ,ps ...) ,body) (Def name (Lambda ps body))]))

(define (parse-program sexps)
  (match sexps
	[`(,ds ... ,body) (Program (map parse-def ds) (parse-exp body))]))

(define (unparse-exp e)
  (match e
    [(Var x) x]
    [(Int n) n]
    [(Bool b) b]
    [(Let x rhs body)
     `(let ([,x ,(unparse-exp rhs)]) ,(unparse-exp body))]
	[(If pred thn els) (cons 'if (map unparse-exp (list pred thn els)))]
    [(Lambda ps body)
     `(lambda ,ps ,(unparse-exp body))]
	[(Begin es e) (cons 'begin (map unparse-exp (append es e)))]
	[(SetBang v e) `(set! ,v ,(unparse-exp e))]
    [(Apply e es) `(,(unparse-exp e) ,@(map unparse-exp es))]
    ))

(define (atm? e)
  (match e
	[(or (? Int?) (? Var?) (? Bool?) (? Quote?)) #t]
	[_ #f]))


(define (fmap-expr f expr)
  (match expr
	[(If e1 e2 e3) (If (f e1) (f e2) (f e3))]
	[(Apply func args) (Apply (f func) (map f args))]
	[(Let v e body) (Let v (f e) (f body))]
	[(Lambda ps body) (Lambda ps (f body))]
	[(SetBang v e) (SetBang v (f e))]
	[(Begin es e) (Begin (map f es) (f e))]
	))

(define (fold-expr f init expr)
  (let ([fold-recur (lambda (e acc) (fold-expr f acc e))])
	(match expr
	  [(If e1 e2 e3) (foldl fold-recur init (list e1 e3 e3))]
	  [(Apply f args) (foldl fold-recur init (cons f args))]
	  [(Let x e body) (foldl fold-recur init (list e body))]
	  [(Lambda `([,xs : ,ts] ...) rty body) (fold-expr f init body)] 
	  [(SetBang v e)  (fold-expr f init e)]
	  [(Begin es body) (foldl fold-recur init (cons body  es))]
	  [(or (? atm?)) init])))

(define-syntax-rule (map-program-exp f p)
  (match-let ([(Program ds body) p])
	(Program 
	  (for/list ([def ds])
		(match-let ([(Def name info body) def])
		  (Def name info (f body))))
	  (f body))))

(define (uniqify p) 
  (define ((uniqify-exp env) e)
	(define (recur e) (uniqify-exp env))
	(match e
	  [(Var v) (dict-ref env v)]
	  [(Let v e body)
	   (let ([v^ (gensym v)])
		 (Let v^ (uniqify-exp e) ((uniqify-exp (cons (cons v v^) env)) body)))]
	  [(Lambda ps body) (uniqify-exp body (map cons ps ps))]
	  [_ (fmap-expr recur e)]))

  (map-program-exp (uniqify-exp '()) p))


(define (hoist-complex-datum p)
  (define (convert d)
	(cond
	  [(pair? d) `(cons ,(convert (car d)) ,(convert (cdr d)))]
	  [else d]))

  (define (convert-expr e callback)
	(match e
	  [(Datum dat) 
	   (cond
		 [(pair? dat) (callback (convert dat))]
		 [else acc])]
	  [else (fmap-expr convert-expr e)]))

  (let* ([collected (make-hash)]
		 [memo 
		   (lambda (cvted) 
			 (let ([v (gensym 'globalDatum)])
			   (dict-set! collected v cvted)
			   (Var v)))])
	(let* ([p^ (map-program-exp (lambda (e) (convert-expr e memo)) p)])
	  (let-values ([(datum-defs datum-inits)
					(for/lists (l1 l2) 
							   ([(v d) (in-hash collected)])
							   (values (Def v (Int 0))
									   (SetBang v d)))])
	  (Program (append datum-defs (Program-def* p^))
			   (Begin datum-defs (Program-body p^)))))))

(define (assignment-conversion p) p)


(define passes
  (list 
	uniqify-exp
	hoist-complex-datum
	assignment-conversion))

(define (write-program p)
  (for ([d (Program-def* p)])
	   (write `(define ,(Def-name d) ,(unparse (Def-body d)))))
  (write (unparse (Program-body p))))


