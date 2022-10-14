(letrec ([f (lambda (g x) (g x))]
		 [a 5]
		 [b (+ 5 7)]
		 [g (lambda (h) (f h 5))]
		 [c (let ([x 10]) ((letrec ([zero? (lambda (n) (= n 0))]
									[f (lambda (n)
										 (if (zero? n)
										   1
										   (* n (f (- n 1)))))])
							 f)
						   x))]
		 [m 10]
		 [z (lambda (x) x)])
  (begin 
  (set! z (lambda (x) (+ x x)))
  (set! m (+ m m))
  (+ (+ (+ (f z a) (f z b)) (f z c)) (g z))))

