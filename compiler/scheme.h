#pragma once

#include <cstdint>
#include <vector>
#include <unordered_set>

namespace Scheme
{
	using ValueType = int64_t;

	enum class Tag : unsigned
	{
		Fixnum 		= 0b000,
		Pair 		= 0b001,
		Vector 		= 0b010,
		Closure 	= 0b011,
		Box 		= 0b100,
		Symbol		= 0b110,
		Bool		= 0b00101, 
		True		= 0b100101,
		False		= 0b000101,
		Nil			= 0b01101,
		Void		= 0b10101,
	};

	enum class Mask : unsigned
	{
		Fixnum		= 0b111,
		Pair 		= 0b111,
		Vector 		= 0b111,
		Closure 	= 0b111,
		Box 		= 0b111,
		Symbol		= 0b111,
		Bool		= 0b11111,
		True		= 0b111111,
		False		= 0b111111,
		Nil			= 0b11111,
		Void		= 0b11111
	};


	inline ValueType toFixnumReps(int64_t n) {
		return n << 3;
	}

	inline ValueType toBoolReps(bool b) {
		return b? static_cast<ValueType>(Tag::True): static_cast<ValueType>(Tag::False);
	}

	static const std::unordered_set<std::string> primitives
	{ 
		"+", "-", "*", "/", 
		">", ">=", "<", "<=", "=",
		"cons", "car", "cdr",
		"box", "unbox", "box-set!", "box?",
		"vector", "make-vector", "vector-ref", "vector-set!", "vector-length", "vector?",
		"null?" , "pair?" ,"symbol?" , "number?" , "boolean?", "eq?", "void?"
	};


};
