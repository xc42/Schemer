(let ((cnt 0))
  (let ((inc (lambda () (set! cnt (+ cnt 1)))))
	(begin (inc) (inc) cnt)))
