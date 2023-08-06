(define (myCons x y)
  (lambda (c)
	(if (= c 0)
	  x
	  y)))

(define (myCar l)
  (l 0))

(define (myCdr l)
  (l 1))

(let ((l (myCons 1 (myCons 2 (myCons 3 4)))))
  (+ (myCar l)
	 (+ (myCar (myCdr l))
		(myCar (myCdr (myCdr l))))))
