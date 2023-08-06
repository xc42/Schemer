(let ((b (box 7)))
  (begin
	  (set-box! b (* (unbox b) (unbox b)))
	  (unbox b)))
