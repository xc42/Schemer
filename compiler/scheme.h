#pragma once

#include <cstdint>

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
		Bool		= 0b1101, 
		True		= 0b11101,
		False		= 0b01101,
		Nil			= 0b000101,
		Void		= 0b100101
	};

	enum class Mask : unsigned
	{
		Fixnum		= 0b111,
		Pair 		= 0b111,
		Vector 		= 0b111,
		Closure 	= 0b111,
		Box 		= 0b111,
		Bool		= 0b1111, //true 0b11101 false 0b01101
		Nil			= 0b111111,
		Void		= 0b111111
	};

	//object allocate on heap should at least 8 bytes aligned
	//because we use lowe 3 bit for tagging
	struct alignas(8) Cons
	{
		ValueType car, cdr;
	};

	struct alignas(8) Vec
	{
		std::size_t len;
		ValueType   *arr;
	};
};
