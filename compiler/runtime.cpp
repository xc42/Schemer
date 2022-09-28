#include "runtime.h"
#include "scheme.h"
#include <iostream>
#include <sstream>

using namespace std;

#define IsSchemeType(val, Ty) (((val) & static_cast<unsigned>(Scheme::Tag::Ty)) == static_cast<unsigned>(Scheme::Mask::Ty))

#define ToConsPtr(val) reinterpret_cast<Scheme::Cons*>(val & ~static_cast<SchemeValTy>(Scheme::Mask::Pair))

static ostream& ToString(ostream& os, SchemeValTy val)
{
	switch( val & 0b111 ) {
		case 0b000:
		{
			os << (val >> 3);
			break;
		}
		case 0b001:
		{
			auto cons = ToConsPtr(val);
			os << "(";
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
			}
			break;
		}
		//TODO
		default:
		{
			cout << "unknown scheme type" << endl;
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
	auto pr = new Scheme::Cons{v1, v2};
	return reinterpret_cast<SchemeValTy>(pr) & static_cast<SchemeValTy>(Scheme::Tag::Pair);
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

SchemeValTy make_vector(SchemeValTy, SchemeValTy){ return 0;}
SchemeValTy vector_ref(SchemeValTy, SchemeValTy){ return 0;}
SchemeValTy vector_length(SchemeValTy){ return 0;}
SchemeValTy vector_set(SchemeValTy, SchemeValTy, SchemeValTy){ return 0;}
