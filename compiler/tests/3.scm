(define (add n) (lambda (x) (+ n x)))
(let ((add2 (add 2)) (add3 (add 3)))
  (add2 (add3 4)))
