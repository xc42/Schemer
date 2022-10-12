(define (has-tag? sexp tag)
  (and (pair? sexp) (eq? (car sexp) tag)))


(and (has-tag? '(if #t thn els) 'if)
	 (not (has-tag? '(foo) 'bar)))
