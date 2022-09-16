#include "parser.h"
#include "interpreter.h"
#include <iostream>

using namespace std;

void repl(const Environment::Ptr env) {
    const std::string input_promt{"~> "};
    Evaluator eval{env};
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

        Expr::Ptr expr;
        try { //parse
            auto tokens = tokenize(line.c_str());
            auto start = tokens.begin(), end = tokens.end();
            expr =  parse(start, end);
        }catch(ParseException& e) { 
            std::cout << e.what() << '\n';
		}catch(std::exception& e) {
            std::cout << e.what() << '\n';
        }

        if(expr != nullptr) {
            try { //eval
                expr->accept(eval);
				eval.getResult()->accept(printer);
				cout << endl;
            }
            catch(std::exception &e) {
                std::cout << e.what() << "\n" ;
            }
        }
    }
}

int main()
{
    repl(builtin::getInitialTopEnv());
}
