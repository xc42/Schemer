#include "codegen.h"
#include <iostream>
#include "parser.h"
#include "front-end-pass.h"

using namespace std;

using namespace Parser;



int main()
{
	cin >> std::noskipws;

	istream_iterator<char> inBegin{std::cin};
	istream_iterator<char> inEnd;
	string src(inBegin, inEnd);

	auto tokens = Parser::tokenize(src.c_str());
	auto prog = FrontEndPass::parseProg(Parser::Range(tokens.begin(),tokens.end()));

	if(prog) {
		auto& p = *prog.getValue();
		FrontEndPass::runAllPass(p);
		ProgramCodeGen gen;
		try {
			gen.gen(p);
			gen.printIR();
		}catch(exception& ex) {
			cerr << "Codegen failed:" << ex.what() << endl;
		}
	}else {
		std::cerr << "ParseError: " <<  prog.getErr() << std::endl;
	}
}
