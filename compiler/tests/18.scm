(let ([vec (make-vector 1024 4)]
	  [bx (box 'abc)])
  (and (vector? vec) (not (box? vec)) (not (number? vec))
	   (box? bx) (not (vector? bx)) (not (boolean? bx))
	   (void? (set-box! bx 233))))
