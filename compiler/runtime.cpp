#include "runtime.h"
#include "scheme.h"
#include <iostream>
#include <sstream>

using namespace std;

#define IsSchemeType(val, Ty) (((val) & static_cast<unsigned>(Scheme::Tag::Ty)) == static_cast<unsigned>(Scheme::Mask::Ty))
#define TagSchemeVal(val, Ty) ((val) | static_cast<unsigned>(Scheme::Tag::Ty))

#define ToConsPtr(val) reinterpret_cast<Scheme::Cons*>(val & ~static_cast<SchemeValTy>(Scheme::Mask::Pair))

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
				os << ")";
			}
			break;
		}
		 //Vector
		case 0b010: { os << "TODO" ; break;}
		//closure
		case 0b011: { os << "#<procedure>"; break;}
		//box
		case 0b100: { os << "TODO" ; break;}
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
		case 0b110: { os << "TODO"; } //TODO
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
	auto pr = new Scheme::Cons{v1, v2};
	return TagSchemeVal(reinterpret_cast<SchemeValTy>(pr), Pair);
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
