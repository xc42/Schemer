#include "runtime.h"
#include "scheme.h"
#include <iostream>

using namespace std;

#define IsSchemeType(val, Ty) (((val) & static_cast<unsigned>(Scheme::Tag::Ty)) == static_cast<unsigned>(Scheme::Mask::Ty))

SchemeValTy display(SchemeValTy val)
{ 
	switch( val & 0b111 ) {
		case 0b000:
		{
			cout << (val >> 3) << endl;
			break;
		}
		case 0b001:
		{
			cout << "unimpl" << endl;
		}
		//TODO
		default:
		{
			cout << "unknown scheme type" << endl;
		}
	}

	return static_cast<SchemeValTy>(Scheme::Tag::Void);
}

SchemeValTy cons(SchemeValTy, SchemeValTy){ return 0;}
SchemeValTy car(SchemeValTy){ return 0;}
SchemeValTy cdr(SchemeValTy){ return 0;}
SchemeValTy make_vector(SchemeValTy, SchemeValTy){ return 0;}
SchemeValTy vector_ref(SchemeValTy, SchemeValTy){ return 0;}
SchemeValTy vector_length(SchemeValTy){ return 0;}
SchemeValTy vector_set(SchemeValTy, SchemeValTy, SchemeValTy){ return 0;}
