(letrec ((even?
          (lambda (n)
            (if (= 0 n)
                #t
                (odd? (- n 1)))))
         (odd?
          (lambda (n)
            (if (= 0 n)
                #f
                (even? (- n 1))))))
  (even? 88))   
