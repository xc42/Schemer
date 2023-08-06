(let ((v (make-vector 2 -1)))
  (begin
    (vector-set! v 0 11)
    (vector-set! v 1 44)
    (+ (vector-ref v 0) (vector-ref v 1))))
