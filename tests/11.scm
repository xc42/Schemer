(define (map2 bin xs ys)
  (if (or (null? xs) (null? ys))
	'()
	(cons (bin (car xs) (car ys)) (map2 bin (cdr xs) (cdr ys)))))

(map2 + '(1 2 3 4 5) '(10 9 8 7 6))
