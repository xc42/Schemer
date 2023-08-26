(define globalDatum2803 0)

(define globalDatum2802 0)

(define globalDatum2801 0)

(define globalDatum2800 0)

(define (has-tag? sexp2798 tag2799) (if (pair? sexp2798) (if (eq? (car sexp2798) tag2799) #t #f) #f))

(begin (set! globalDatum2803 (quote bar)) (set! globalDatum2802 (cons (quote foo) (quote ()))) (set! globalDatum2801 (quote if)) (set! globalDatum2800 (cons (quote if) (cons #t (cons (quote thn) (cons (quote els) (quote ())))))) (if (has-tag? globalDatum2800 globalDatum2801) (if (if (has-tag? globalDatum2802 globalDatum2803) #f #t) #t #f) #f))
