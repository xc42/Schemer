#include "runtime.h"
#include "scheme.h"
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <memory>

using namespace std;

#define IsSchemeType(val, Ty) (((val) & static_cast<unsigned>(Scheme::Mask::Ty)) == static_cast<unsigned>(Scheme::Tag::Ty))
#define TagSchemeVal(val, Ty) (reinterpret_cast<SchemeValTy>(val) | static_cast<unsigned>(Scheme::Tag::Ty))

#define ToConsPtr(val) reinterpret_cast<Runtime::Cons*>((val) & ~static_cast<SchemeValTy>(Scheme::Mask::Pair))

#define ToVecPtr(val) reinterpret_cast<Runtime::Vec*>((val) & ~static_cast<SchemeValTy>(Scheme::Mask::Vector))
#define ToBoxPtr(val) reinterpret_cast<Runtime::Box*>((val) & ~static_cast<SchemeValTy>(Scheme::Mask::Box))

static ostream& ToString(ostream& os, SchemeValTy val)
{
	switch( val & 0b111 ) {
		case 0b000: //Fixnum
		{
			os << (val >> 3);
			break;
		}
		case 0b001: //Pair
		{
			auto cons = ToConsPtr(val);
			os << "'(";
			ToString(os, cons->car);

			auto it = cons->cdr;
			while(IsSchemeType(it, Pair)) {
				auto p = ToConsPtr(it);
				os << " ";
				ToString(os, p->car);
				it = p->cdr;
			}

			if(IsSchemeType(it, Nil)) {
				os << ")";
			}else {
				os << " . ";
				ToString(os, it);
				os << ")";
			}
			break;
		}
		 //Vector
		case 0b010: 
		{ 
			auto v = ToVecPtr(val);
			auto len = v->len;
			os << "'#(";
			for(int i = 0; i < len - 1; ++i) {
				ToString(os, v->arr[i]);
				os << ' ';
			}
			if(len >= 1) ToString(os, v->arr[len-1]);
			os << ")";
			break;
		}
		//closure
		case 0b011: { os << "#<procedure>"; break;}
		//box
		case 0b100:
		{ 
			os << "'#&";
			auto p = ToBoxPtr(val);
			ToString(os, p->val);
			break;
		}
		//various subtype
		case 0b101:
		{
			switch(val & 0b11111) {
				//bool
				case 0b00101: { os << ((val & 0b100000)? "#t": "#f"); break;}
				//Nil
				case 0b01101: { os << "()"; break;}
				//void
				case 0b10101: { os << "#void"; break;}

				default:
					os << "#unknown type";
			}
			break;
		}
		//symbol
		case 0b110:
		{
			auto sym = reinterpret_cast<Runtime::Sym*>(val & ~static_cast<SchemeValTy>(Scheme::Mask::Symbol));
			os << "'" << sym->name;
			break;
		}
		default:
		{
			os << "#unknown type" ;
		}
	}
	return os;
}

SchemeValTy display(SchemeValTy val)
{ 
	ostringstream oss;
	ToString(oss, val);
	cout << oss.str() << endl;
	return static_cast<SchemeValTy>(Scheme::Tag::Void);
}

SchemeValTy cons(SchemeValTy v1, SchemeValTy v2)
{
	auto pr = new Runtime::Cons{v1, v2};
	return TagSchemeVal(pr, Pair);
}

SchemeValTy car(SchemeValTy pr)
{
	auto addr = ToConsPtr(pr);
	return addr->car;
}

SchemeValTy cdr(SchemeValTy pr)
{
	auto addr = ToConsPtr(pr);
	return addr->cdr;
}

SchemeValTy box(SchemeValTy val)
{
	auto b = new Runtime::Box;
	b->val = val;
	return TagSchemeVal(b, Box);
}

SchemeValTy unbox(SchemeValTy b)
{
	auto p = ToBoxPtr(b);
	return p->val;
}

SchemeValTy set_45_box_33_(SchemeValTy b, SchemeValTy val)
{
	auto p = ToBoxPtr(b);
	p->val = val;
	return static_cast<SchemeValTy>(Scheme::Tag::Void);
}

SchemeValTy  make_45_vector(SchemeValTy len, SchemeValTy val)
{
	auto v = new Runtime::Vec;
	v->len = len >> 3;
	v->arr = new SchemeValTy [v->len];
	std::fill(v->arr, v->arr + v->len, val);
	return TagSchemeVal(v, Vector);
}

SchemeValTy  vector_45_ref(SchemeValTy v, SchemeValTy idx)
{
	auto vp = ToVecPtr(v);
	return vp->arr[idx];
}

SchemeValTy vector_45_length(SchemeValTy v)
{ 
	auto vp = ToVecPtr(v);
	return Scheme::toFixnumReps(vp->len);
}

SchemeValTy vector_45_set_33_(SchemeValTy v, SchemeValTy idx, SchemeValTy val)
{ 
	auto vp = ToVecPtr(v);
	vp->arr[idx] = val;
	return static_cast<SchemeValTy>(Scheme::Tag::Void);
}

SchemeValTy schemeAllocateClosure(char* code, int arity, int fvs, ...)
{
	auto clos = new Runtime::Closure;
	clos->code = code;
	clos->arity = arity;
	clos->fvs = new SchemeValTy [fvs];

	va_list vargs;
	va_start(vargs, fvs);
	for(int i = 0; i < fvs; ++i) {
		clos->fvs[i] = va_arg(vargs, Scheme::ValueType);
	}
	va_end(vargs);

	return TagSchemeVal(clos, Closure);
}

SchemeValTy schemeInternSymbol(const char* sym)
{
	static unordered_map<string, unique_ptr<Runtime::Sym>> pool;

	auto it = pool.find(sym);
	Runtime::Sym* ret = nullptr;
	if(it == pool.end()) {
		auto it = pool.insert({sym, make_unique<Runtime::Sym>(sym)});
		ret = it.first->second.get();
	}else {
		ret = it->second.get();
	}
	return TagSchemeVal(ret, Symbol);
}

SchemeValTy null_63_(SchemeValTy val)
{
	return Scheme::toBoolReps((val & static_cast<SchemeValTy>(Scheme::Mask::Nil)) == static_cast<SchemeValTy>(Scheme::Tag::Nil));
}

SchemeValTy pair_63_(SchemeValTy val)
{
	return Scheme::toBoolReps((val & static_cast<SchemeValTy>(Scheme::Mask::Pair)) == static_cast<SchemeValTy>(Scheme::Tag::Pair));
}

SchemeValTy symbol_63_(SchemeValTy val)
{
	return Scheme::toBoolReps((val & static_cast<SchemeValTy>(Scheme::Mask::Symbol)) == static_cast<SchemeValTy>(Scheme::Tag::Symbol));
}

SchemeValTy number_63_(SchemeValTy val)
{
	return Scheme::toBoolReps((val & static_cast<SchemeValTy>(Scheme::Mask::Fixnum)) == static_cast<SchemeValTy>(Scheme::Tag::Fixnum));
}

SchemeValTy eq_63_(SchemeValTy v1, SchemeValTy v2)
{
	return Scheme::toBoolReps(v1 == v2);
}

namespace Runtime
{


}
