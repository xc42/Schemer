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
	SchemeValTy box_63_(SchemeValTy);
	SchemeValTy unbox(SchemeValTy);
	SchemeValTy set_45_box_33_(SchemeValTy,SchemeValTy);

	SchemeValTy make_45_vector(SchemeValTy, SchemeValTy);
	SchemeValTy vector_63_(SchemeValTy);
	SchemeValTy vector_45_ref(SchemeValTy, SchemeValTy);
	SchemeValTy vector_45_length(SchemeValTy);
	SchemeValTy vector_45_set_33_(SchemeValTy, SchemeValTy, SchemeValTy);

	SchemeValTy null_63_(SchemeValTy);
	SchemeValTy pair_63_(SchemeValTy);
	SchemeValTy symbol_63_(SchemeValTy);
	SchemeValTy number_63_(SchemeValTy);
	SchemeValTy boolean_63_(SchemeValTy);
	SchemeValTy eq_63_(SchemeValTy, SchemeValTy);
	SchemeValTy void_63_(SchemeValTy);

	SchemeValTy schemeAllocateClosure(char* code, int arity, int fvs, ...);
	SchemeValTy schemeInternSymbol(const char* sym); 
}

namespace Runtime
{
	//object allocate on heap should at least 8 bytes aligned
	//because we use lower 3 bit for tagging
	struct alignas(8) Cons
	{
		Scheme::ValueType car, cdr;
	};

	struct alignas(8) Vec
	{
		std::size_t len;
		Scheme::ValueType   *arr;
	};

	struct alignas(8) Box
	{
		Scheme::ValueType val;
	};

	struct alignas(8) Sym
	{
		Sym(const char* c): name(c) {}
		const char *name;
	};

	struct alignas(8) Closure
	{
		int arity;
		char* code;
		Scheme::ValueType *fvs;
	};

	static const std::unordered_map<std::string, int> builtinFunc = 
	{ 
		//name, arity
		{"display", 1},
		{"cons", 2},
		{"car", 1},
		{"cdr", 1},
		{"box", 1}, {"box?", 1}, {"unbox", 1}, {"set-box!", 2},
		{"make-vector", 2}, {"vector-ref", 2}, {"vector-length", 1}, {"vector-set!", 3}, {"vector?", 1},
		{"null?", 1}, {"void?", 1}, {"pair?", 1},{"symbol?", 1}, {"number?", 1}, {"boolean?", 1}, {"eq?", 2}
	};

}
