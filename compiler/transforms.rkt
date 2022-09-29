#lang racket

(struct Int (value)) 
(struct Bool (value)) 
(struct Var (name)) 
(struct Let (var rhs body)) 
(struct If (cnd thn els)) 
(struct Lambda (param* body))
(struct Define (name body))

(define (parse-exp e)
    (match e
      [(? symbol?) (Var e)]
      [(? fixnum?) (Int e)]
      [(? boolean?) (Bool e)]
      [`(let ([,xs ,rhs] ...) ,body)  '()]

      [`(if ,cnd ,thn ,els) (If (parse-exp cnd) (parse-exp thn) (parse-exp els))]
      [`(cond (,pred ,body) ..1 (else ,els))
        (for/foldr ([acc (parse-exp els)])
                   ([p pred]
                    [b body])
                   (If (parse-exp p) (parse-exp b) acc))]

      [`(lambda ,ps ,body) 
       (Lambda ps (parse-exp body))]))




(define (uniqify p) '())

(define (hoist-complex-datum p) '())

(define (assignment-conversion p) '())


