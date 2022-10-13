(define (*id* x) x)

(let ((f *id*))
  (((f f) f) 42))
