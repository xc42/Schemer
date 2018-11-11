# Lisp Interpreter in C++

A simple implementation of Lisp(actually more like scheme) in C++,this work was initially 
done when I finished the first section of 4th chapter of SICP(Structure and Interpretation 
of Computer Programs).For simplicity, this interpreter doesn't support real/float numbers 
,strings and big numbers, but have a minmum core to correctly evaluate many of my SICP 
exercises

## Current supports

- basic operator **quote**  , **if** , **and** , **or**
- basic function **cons** , **car** , **cdr**, **list**, **eval**, **apply**
- basic test function **eq?** , **number?** , **pair?** , **symbol?** , **null?** (using #t,#f to represent true and false)
- using "lambda" to define function and lambda body can have multiple expression
- using "define" to bound symbol in env and its syntactic sugar form to define function

  e.g:

  ``` lisp
   (define square (lambda (x) (* x x)))
   (define (fac n) (if (= 0 n) 1 (* n (fac (- n 1)))))
   ;also supports inner define
   (define (fac n)
     (define (fac-iter acc n)
       (if (= 0 n)
         acc
         (fac-iter (* n acc) (- n 1))))
    (fac-iter 1 n))
   ```
- basic arithmetical operator,+, -, *, /, %(for mod), comparator>, <, =, >=, <=.


