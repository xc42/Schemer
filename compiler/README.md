# Scheme (subset) Compiler (LLVM as backend)

## current language
````
Expr      --> <Primitive>
		   |  <Var>
		   |  <Const>
		   |  (quote <Datum>)
		   |  (if <Expr> <Expr> <Expr>)
		   |  (or <Expr> ...)
		   |  (and <Expr> ...)
		   |  (not <Expr>)
		   |  (begin <Expr> ... <Expr>)
		   |  (lambda (<Var> ...) <Expr> ... <Expr>)
		   |  (let ([<Var> <Expr>] ...) <Expr> ... <Expr>)
		   |  (letrec ([<Var> <Expr>] ...) <Expr> ... <Expr>)
		   |  (set! <Var> <Expr>)
		   |  (<Expr> <Expr> ...)

Primitive --> car | cdr | cons | pair? 
		   |  null? | boolean? | symbol? | number? | void?
		   |  make-vector | vector-ref | vector-set! | vector? | vector-length
		   |  box | unbox | set-box! | box? 
		   |  + | - | * | / | = | < | <= | > | >= | eq?

Var       --> symbol
Const     --> #t | #f | '() | integer between -2^60 and 2^60 - 1
Datum     --> <Const> | (<Datum> . <Datum>) | #(<Datum> ...)
````