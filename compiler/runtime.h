#pragma once

#include "scheme.h"

extern "C"
{
	using SchemeValTy = Scheme::ValueType;
	SchemeValTy display(SchemeValTy);
	SchemeValTy cons(SchemeValTy, SchemeValTy);
	SchemeValTy car(SchemeValTy);
	SchemeValTy cdr(SchemeValTy);
	SchemeValTy make_vector(SchemeValTy, SchemeValTy);
	SchemeValTy vector_ref(SchemeValTy, SchemeValTy);
	SchemeValTy vector_length(SchemeValTy);
	SchemeValTy vector_set(SchemeValTy, SchemeValTy, SchemeValTy);
}

