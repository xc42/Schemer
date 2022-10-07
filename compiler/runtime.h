#pragma once

#include "scheme.h"
#include <unordered_map>

extern "C"
{
	using SchemeValTy = Scheme::ValueType;
	SchemeValTy display(SchemeValTy);
	SchemeValTy cons(SchemeValTy, SchemeValTy);
	SchemeValTy car(SchemeValTy);
	SchemeValTy cdr(SchemeValTy);

	// the '-' are "mangled", see ProgramCodeGen::simpleMangle
	
	SchemeValTy box(SchemeValTy);
	SchemeValTy unbox(SchemeValTy);
	SchemeValTy set_45_box_33_(SchemeValTy,SchemeValTy);
	SchemeValTy make_45_vector(SchemeValTy, SchemeValTy);
	SchemeValTy vector_45_ref(SchemeValTy, SchemeValTy);
	SchemeValTy vector_45_length(SchemeValTy);

	SchemeValTy vector_45_set_33_(SchemeValTy, SchemeValTy, SchemeValTy);

	SchemeValTy allocateClosure(char* code, int arity, int fvs, ...);
}

namespace Runtime
{

static const std::unordered_map<std::string, int> builtinFunc = 
{ 
	//name, arity
	{"display", 1},
	{"cons", 2},
	{"car", 1},
	{"cdr", 1},
	{"box", 1},
	{"unbox", 1},
	{"set-box!", 2},
	{"make-vector", 2},
	{"vector-ref", 2},
	{"vector-length", 1},
	{"vector-set!", 3},
};

}
