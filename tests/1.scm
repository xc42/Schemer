(define (range l h) 
  (if (< l h) 
	  (cons l (range (+ l 1) h))
	  '()))


(range 0 10)
