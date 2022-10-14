#! /bin/racket
#lang racket

(module AST racket 
  (struct/contract Expr () #:transparent)
  (struct/contract Int Expr ([value fixnum?])  #:transparent)
  (struct/contract Bool Expr ([value boolean?])  #:transparent)
  (struct/contract Var Expr ([name symbol?])  #:transparent)
  (struct/contract Quote Expr ([dat (or/c pair? symbol? fixnum? null? boolean?)]) #:transparent)
  (struct/contract Let Expr ([var symbol?] [rhs Expr?] [body Expr?])  #:transparent)
  (struct/contract LetRec Expr ([binds* (listof (cons/c symbol? Expr?))] [body Expr?])  #:transparent)
  (struct/contract If Expr ([cnd Expr?] [thn Expr?] [els Expr?])  #:transparent)
  (struct/contract Lambda Expr ([param* (listof symbol?)] [body Expr?]) #:transparent)
  (struct/contract Prim Expr ([op symbol?] [es (listof Expr?)]) #:transparent)
  (struct/contract Apply Expr ([fun Expr?] [arg* (listof Expr?)]) #:transparent)
  (struct/contract Begin Expr ([es (listof Expr?)] [e Expr?]) #:transparent   )
  (struct/contract SetBang Expr ([var symbol?] [e Expr?]) #:transparent)
  (struct/contract Def ([name symbol?] [info any/c] [body Expr?]) #:transparent)
  (struct/contract Program ([def* (listof Def?)] [body Expr?]) #:transparent )

  (provide (all-defined-out))
)

(require 'AST)

(define primitives
  (make-hash 
	'((+ . 2) (- . 2) (* . 2) (/ . 2) 
	  (> . 2) (>= . 2) (< . 2) (<= . 2) (= . 2)
	  (cons . 2) (car . 1) (cdr . 1)
	  (make-vector . 2) (vector-set! . 3) (vector-ref . 2) (vector-length . 1)
	  (box . 1) (set-box! 2) (unbox . 1)
	  (null? . 1) (pair? . 1) (symbol? . 1) (number? . 1) (eq? . 2))))

(define (parse-exp e)
  (match e
	[(? symbol?) (Var e)]
	[(? fixnum?) (Int e)]
	[(? boolean?) (Bool e)]
	[`(let ([,xs ,rhs] ...) ,body)
	;;TODO 这里其实是当作let*的语义了,暂时没决定好怎么处理多个绑定的let
	  (for/foldr ([acc (parse-exp body)])
				 ([x xs]
				  [r rhs])
				 (Let x (parse-exp r) acc))]
	[`(letrec ([,xs ,es] ...) ,body)
	  (LetRec (for/list ([x xs] [e es]) (cons x (parse-exp e))) (parse-exp body))]
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
	[`(not ,e) (If (parse-exp e) (Bool #f) (Bool #t))] 
	[`(and ,es ...)
	  (for/foldr ([thn (Bool #t)])
		([e es])
		(If (parse-exp e)
			thn
			(Bool #f)))]
	[`(or ,es ...)
	  (for/foldr ([els (Bool #f)])
		([e es])
		(If (parse-exp e)
			(Bool #t)
			els))]
	[`(,rator ,rands ...) 
	  (cond
		[(hash-has-key? primitives rator) (Prim rator (map parse-exp rands))]
		[else (Apply (parse-exp rator) (map parse-exp rands))])]
	;[_ (error "parse-exp: unknown exp")]
	))

(define (parse-def d)
  (match d
	[`(define ,(list name ps ...) ,body) (Def name '() (Lambda ps (parse-exp body)))]
	[`(define ,name ,body) (Def name '() (parse-exp body))]))

(define (parse-program sexps)
  (match sexps
	[`(,ds ... ,body) (Program (map parse-def ds) (parse-exp body))]))

(define (unparse-exp e)
  (match e
    [(Var x) x]
    [(Int n) n]
    [(Bool b) b]
	[(Quote dat) `(quote ,dat)]
    [(Let x rhs body)
     `(let ([,x ,(unparse-exp rhs)]) ,(unparse-exp body))]
	[(LetRec binds* body)
	 `(letrec ,(map (lambda (bd) (list (car bd) (cdr bd))) binds*) ,(unparse-exp body))]
	[(If pred thn els) (cons 'if (map unparse-exp (list pred thn els)))]
    [(Lambda ps body)
     `(lambda ,ps ,(unparse-exp body))]
	[(Begin es e) `(begin ,@(map unparse-exp es) ,(unparse-exp e))]
	[(SetBang v e) `(set! ,v ,(unparse-exp e))]
	[(Prim op es) `(,op ,@(map unparse-exp es))]
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
	[(Prim op es) (Prim op (map f es))]
	[(Let v e body) (Let v (f e) (f body))]
	[(Lambda ps body) (Lambda ps (f body))]
	[(SetBang v e) (SetBang v (f e))]
	[(Begin es e) (Begin (map f es) (f e))]
	[(? atm?) expr]
	))

(define (fold-expr f init expr)
	(match expr
	  [(If e1 e2 e3) (foldl f init (list e1 e3 e3))]
	  [(Apply func args) (foldl f init (cons func args))]
	  [(Prim op es) (foldl f init es)]
	  [(Let x e body) (foldl f init (list e body))]
	  [(LetRec binds* body) (foldl (lambda (bd acc) (f (cdr bd) acc)) (f body init) binds*)]
	  [(Lambda ps body) (f body init)]
	  [(SetBang v e)  (f e init)]
	  [(Begin es body) (foldl f init (cons body  es))]
	  [(? atm?) init]))

(define-syntax-rule (map-program-exp f p)
  (match-let ([(Program ds body) p])
	(Program 
	  (for/list ([def ds])
		(match-let ([(Def name info body) def])
		  (Def name info (f body))))
	  (f body))))

(define (uniqify p) 
  (define ((uniqify-exp env) e)
	(let ([recur (uniqify-exp env)])
	  (match e
		[(Var v) (Var (dict-ref env v))]
		[(Let v e body)
		 (cond 
		   [(dict-ref env v #f)
			=> (lambda (v)
				 (let* ([v^ (gensym v)]
						[env^ (cons (cons v v^) env)])
				   (Let v^ (recur e) ((uniqify-exp env^) body))))]
		   [else (Let v (recur e) ((uniqify-exp (cons (cons v v) env)) body))])]
		[(LetRec binds* body) 
		 (let* ([env^ (for/fold ([env env]) 
						([bd binds*]) 
						(cons (cons (car bd) (gensym (car bd))) env))]
				[uniq (uniqify-exp  env^)])
		   (LetRec (for/fold ([binds^ '()]) 
					 ([bd binds*])
					 (cons 
					   (cons (dict-ref env^ (car bd)) (uniq (cdr bd)))
					   binds^))
				   (uniq body)))]
		[(Lambda ps body)
		 (let* ([ps^ (map gensym ps)]
				[env^ (for/fold ([env^ env])
						([p ps]
						 [p^ ps^])
						(cons (cons p p^) env^))]
				[body^ ((uniqify-exp env^) body)])
		   (Lambda ps^ body^))]
		[(SetBang v e) (SetBang (dict-ref env v) (recur e))]
		[_ (fmap-expr recur e)])))

  (let* ([top-env
		  (for/list ([d (Program-def* p)]) 
			(cons (Def-name d) (Def-name d)))]
		[init-env 
		  (for/fold ([env top-env])
			([(k v) (in-hash primitives)])
			(cons (cons k k) env))])
  (map-program-exp (uniqify-exp init-env) p)))

(define (convert-letrec p)
  (define ((check-letrec vars) e)
	(match e
	  [(? Lambda?) #t]
	  [(Var v) (not (set-member? vars v))]
	  [_ (fold-expr (lambda (e acc) (and acc ((check-letrec vars) e))) #t e)]))

  (define (convert-expr e)
	(match e
	  [(LetRec binds* body)
	   (let* ([vars (for/set ([bd binds*]) (car bd))]
			  [unref? (check-letrec vars)])
		 (unless (andmap (lambda (bd) (unref? (cdr bd))) binds*)
		   (error "invalid letrec")))

	   (for/fold ([body^ (Begin 
						   (for/list 
							 ([bd binds*]) 
							 (SetBang (car bd) (convert-expr (cdr bd))))
						   body)])
		 ([bd binds*])
		 (Let (car bd) (Int -1) body^))]
	  [_ (fmap-expr convert-expr e)]))

  (map-program-exp convert-expr p))


(define (eta-abstract-prim p) 
  (map-program-exp (eta-abstract-expr primitives) p))

(define (eta-abstract-top-func p)
  (let ([top-func 
		  (for/hash ([d (Program-def* p)]
					 #:when (Lambda? (Def-body d)))
			(values (Def-name d) (length (Lambda-param* (Def-body d)))))])

	(map-program-exp (eta-abstract-expr top-func) p)))

(define ((eta-abstract-expr name-arity) expr)
  (define recur (eta-abstract-expr name-arity))
  (match expr
	[(Var v)
	 (let ([arity (hash-ref name-arity v #f)])
	   (if arity 
		 (let ([ps (build-list arity (lambda (n) (gensym 'x)))]) 
		   (Lambda ps (Apply (Var v) (map Var ps))))
		 expr))]
	[(Apply (Var v) es) (Apply (Var v) (map recur es))]
	[_ (fmap-expr recur expr)]))

(define (hoist-complex-datum p)
  (define (convert d)
	(cond
	  [(pair? d) (Prim 'cons (list (convert (car d)) (convert (cdr d))))]
	  [(fixnum? d) (Int d)]
	  [(boolean? d) (Bool d)]
	  [else (Quote d)]))

  (define ((convert-expr  callback) e)
	(match e
	  [(Quote dat) (callback (convert dat))]
	  [else (fmap-expr (convert-expr callback) e)]))

  (let* ([collected (make-hash)]
		 [memo 
		   (lambda (cvted) 
			 (match cvted
			   [(Quote (or (? null?) (? fixnum?))) cvted]
			   [_ (let ([v (gensym 'globalDatum)])
				   (dict-set! collected v cvted)
				   (Var v))]))])
	(let* ([p^ (map-program-exp (convert-expr memo) p)])
	  (let-values ([(datum-defs datum-inits)
					(for/lists (l1 l2) 
							   ([(v d) (in-hash collected)])
							   (values (Def v '() (Int 0))
									   (SetBang v d)))])
	  (Program (append datum-defs (Program-def* p^))
			   (Begin datum-inits (Program-body p^)))))))

(define (opt-direct-call p)
  (define (opt-expr expr)
	(match expr
	  [(Apply (Lambda ps body) es)
	   (for/fold ([body^ body])
		 ([p ps]
		  [e es])
		 (Let p e body^))]
	  [_ (fmap-expr opt-expr expr)]))

  (map-program-exp opt-expr p))

(define (opt-known-call p)
  (define ((opt-expr env) expr)
	(define opt-recur (opt-expr env))
	(match expr
	  [(Let v (and lam (Lambda ps lam-body)) let-body)
	   (let* ([lam^ (Lambda ps (opt-recur lam-body))]
			  [let-body^ ((opt-expr (cons (cons v lam^) env)) let-body)])
		 (Let v lam^ let-body^))]
	  [(Apply (Var v) es)
	   (cond
		 [(dict-ref env v #f)
		  => (lambda (lam)
			   (for/fold ([e^ (Lambda-body lam)])
				 ([p (Lambda-param* lam)]
				  [e es])
				 (Let p (opt-recur e) e^)))]
		 [else (Apply (Var v) (map opt-recur es))])]
	  [_ (fmap-expr opt-recur expr)]))
  (map-program-exp (opt-expr '()) p))

(define (uncover-free expr)
  (match expr
	[(Var v) (set v)]
	[(Let v e body) (set-union (uncover-free e) (set-remove (uncover-free body) v))]
	[(Lambda ps body) (set-subtract (uncover-free body) (apply set ps))]
	[_ (fold-expr (lambda (e acc) (set-union (uncover-free e) acc)) (set) expr)]))

(define (assignment-conversion p)
  (define (scan-assigned-captured expr)
	(match expr
	  [(SetBang v e) 
	   (let ([ass-cap (scan-assigned-captured e)])
		 (cons (set-add (car ass-cap) v) 
			   (cdr ass-cap)))]
	  [(Lambda ps body)
	   (let ([fvs (uncover-free expr)]
			 [ass-cap (scan-assigned-captured body)])
		 (cons (car ass-cap) (set-union fvs (cdr ass-cap))))]
	  [_ (fold-expr 
		   (lambda (e acc)
			 (match-let ([(cons ass cap) (scan-assigned-captured e)]
						 [(cons acc-ass acc-cap) acc])
			   (cons (set-union ass acc-ass)
					 (set-union cap acc-cap))))
		   (cons (set) (set))
		   expr)]))


  (define (conv-recur expr)
	(let* ([assign-cap (scan-assigned-captured expr)]
		   [box-it? (lambda (v) 
					  (and (set-member? (car assign-cap) v) 
						   (set-member? (cdr assign-cap) v)))])
	  (let recur ([expr expr])
		(match expr
		  [(Var v) (if (box-it? v) (Apply (Var 'unbox) (list expr)) expr)]
		  [(Let v e body) 
		   (let ([e^ (if (box-it? v) 
					   (Apply (Var 'box) (list (recur e)))
					   (recur e))])
			 (Let v e^ (recur body)))]
		  [(SetBang v e) 
		   (if (box-it? v)
			 (Apply (Var 'set-box!) (list (Var v) (recur e)))
			 (SetBang v (recur e)))]
		  [_ (fmap-expr recur expr)]))))

  (map-program-exp conv-recur p))


(define passes
  (list 
	uniqify
	convert-letrec
	eta-abstract-prim 
	eta-abstract-top-func 
	hoist-complex-datum
	opt-direct-call
	opt-known-call
	assignment-conversion
	))

(define (write-program p)
  (for ([d (Program-def* p)])
	(match d
	  [(Def name info (Lambda ps body))
	   (write `(define ,(cons name ps) ,(unparse-exp body)))]
	  [(Def name info body)
	   (write `(define ,name ,(unparse-exp body)))])
	   (newline)
	   (newline))
  (write (unparse-exp (Program-body p)))
  (newline))

(define (read-program [path ""])
  (if (= 0 (string-length path))
	(parse-program (for/list ([e (in-port read)]) e))
	(call-with-input-file
	  path
	  (lambda (f) 
		(parse-program (for/list ([e (in-port read f)]) e))))))
  
(module+ main
  (write-program ((apply compose (reverse passes)) (read-program))))
