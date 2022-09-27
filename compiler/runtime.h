#pragma once

#include <cstdint>

using SchemeValTy = int64_t;
extern "C"
{
	SchemeValTy display(SchemeValTy);
	SchemeValTy cons(SchemeValTy, SchemeValTy);
	SchemeValTy car(SchemeValTy);
	SchemeValTy cdr(SchemeValTy);
	SchemeValTy make_vector(SchemeValTy, SchemeValTy);
	SchemeValTy vector_ref(SchemeValTy, SchemeValTy);
	SchemeValTy vector_length(SchemeValTy);
	SchemeValTy vector_set(SchemeValTy, SchemeValTy, SchemeValTy);
}
