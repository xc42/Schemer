#include "parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
//#include <optional>

using namespace std;
using namespace Interp;

void repl(const EnvironmentPtr env) {
    const std::string input_promt{"~> "};
    Evaluator eval(env);
	ValuePrinter printer(cout);
    for(;;) {
        std::cout << input_promt;
        std::string line;
        int leftParen = 0;
        do {
            std::string next_line;
            std::getline(std::cin, next_line);

            if(std::cin.eof()) {
                std::cout << "\n\nMoriturus te saluto." << std::endl;
                return ;
            }
                
            for(const char c: next_line) {
                if(c == '(') ++leftParen;
                else if(c == ')') --leftParen;
            }
            line += next_line;
        }while(leftParen > 0);

        if(leftParen != 0) {
            std::cout << "parentheses mismatch!\n";
            continue;
        }

		auto tokens = Parser::tokenize(line.begin(), line.end());
		auto expr =  Parser::parseExp(Parser::Range{tokens.begin(), tokens.end()});

        if(expr) {
            try { //eval
                expr.getValue()->accept(eval);
				eval.getResult()->accept(printer);
                cout << endl;
            } catch(std::exception &e) {
                std::cerr << e.what() << "\n" ;
            }
        }else {
			std::cerr << "ParseError: " <<  expr.getErr() << std::endl;
		}
    }
}

void evalSource(const string& src) {
    auto tokens = Parser::tokenize(src.begin(), src.end());
    auto prog =  Parser::parseProgram(Parser::Range{tokens.begin(), tokens.end()});

    if (!prog) {
        std::cerr << "ParseError: " << prog.getErr() << std::endl;
        return;
    }

    Evaluator eval{builtin::getInitialTopEnv()};
	ValuePrinter printer(cout);
    try { //eval
        for (auto& expr: prog.getValue()) {
            expr->accept(eval);
            eval.getResult()->accept(printer);
        }
    } catch(std::exception &e) {
        std::cout << e.what() << "\n" ;
    }

}

int main(int argc, char *argv[])
{
    auto argBegin = argv,  argEnd = argv + argc;
    auto hasOpt = [&](string_view opt, bool next=false) -> char* {
        char **it = std::find_if(argBegin, argEnd, [&](const char* arg) { return arg == opt; });
        if (it != argEnd) {
            return next? (it + 1 != argEnd? *(it+1): nullptr): *it;
        } else {
            return nullptr;
        }
    };


    if (hasOpt("-e")) { //read code from stdin
        string src{std::istreambuf_iterator<char>(cin), {}};
        evalSource(src);
    } else if (auto filepath = hasOpt("-f", true)) {
        ifstream ifs(filepath);
        string src{std::istreambuf_iterator<char>(ifs), {}};
        evalSource(src);
    }else { // enter read-eval-print-loop
        repl(builtin::getInitialTopEnv());
    }
}
