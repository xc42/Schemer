#include "parser.h"
#include "interpreter.h"
#include "bccompiler.h" 
#include "bcdumper.h"
#include "machine.h"
#include <iostream>
#include <fstream>
//#include <optional>

using namespace std;
using namespace Interp;

class EvalShell {
public:
    enum class EngineType { Tree, VM };
    EvalShell(EngineType e): _engineTy(e) {
        if (e == EngineType::Tree) {
            _treeEvaluator = make_unique<Evaluator>(builtin::initTopEnv());
        } else {
            _compiler   = make_unique<ByteCodeCompiler>();
            _vm         = make_unique<VirtualMachine>();
        }
    }

    void loop() {
        auto input_promt = "~> "sv;
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
            auto expr   =  Parser::parseExp(Parser::Range{tokens.begin(), tokens.end()});

            if(expr) {
                auto val = evalExpr(*expr.getValue());
                val->accept(printer);
                cout << endl;
            }else {
                std::cerr << "ParseError: " <<  expr.getErr() << std::endl;
            }
        }
    }

    
    void fromSource(const string& src, bool onlyCmpl = false) {
        auto tokens = Parser::tokenize(src.begin(), src.end());
        auto prog =  Parser::parseProgram(Parser::Range{tokens.begin(), tokens.end()});

        if (!prog) {
            std::cerr << "ParseError: " << prog.getErr() << std::endl;
            return;
        }

        Evaluator eval{builtin::initTopEnv()};
        try { //eval
            if (onlyCmpl) {
                for (auto& expr: prog.getValue()) {
                    compileExpr(*expr);
                }
            } else {
                for (auto& expr: prog.getValue()) {
                    evalExpr(*expr);
                }
            }
        } catch(std::exception &e) {
            std::cout << e.what() << "\n" ;
        }
    }

    Value::Ptr evalExpr(Expr& expr) {
        if (_engineTy == EngineType::Tree) {
            expr.accept(*_treeEvaluator);
            return _treeEvaluator->getResult();
        } else {
            auto instrs = _compiler->Compile(expr);
            return _vm->execute(*instrs);
        }
    }
    void compileExpr(Expr& expr) {
        InstrDumper dumper(cout);
        try { //eval
            auto instrs = _compiler->Compile(expr);
            dumper.dump(*instrs);
        } catch(std::exception &e) {
            std::cerr << e.what() << "\n" ;
        }
    }
private:
    EngineType                      _engineTy;
    unique_ptr<Evaluator>           _treeEvaluator;
    unique_ptr<ByteCodeCompiler>    _compiler;
    unique_ptr<VirtualMachine>      _vm;
};

void help() {
    cout << "schemer [OPTIONS]\n\n";
    cout << "\t[--engine vm|tree] (defalut:vm) change the engine of scheme interpreter" << endl
         << "\t[-e expr] eval expr directly]" << endl
         << "\t[-f filename] eval code from filename]" << endl
         << "\t[-d filename] print the bytecode compiled from filename" << endl;
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

    auto tyOpt      = hasOpt("--engine", true);
    auto engineTy   = tyOpt? ("vm"sv == tyOpt? EvalShell::EngineType::VM: EvalShell::EngineType::Tree):
                      EvalShell::EngineType::VM;

    engineTy        = hasOpt("-d")? EvalShell::EngineType::VM: engineTy;

    EvalShell   shell(engineTy);

    if (hasOpt("-e")) { //read code from stdin
        string src{std::istreambuf_iterator<char>(cin), {}};
        shell.fromSource(src);
    }
    else if (auto filepath = hasOpt("-f", true)) {
        ifstream ifs(filepath);
        string src{std::istreambuf_iterator<char>(ifs), {}};
        shell.fromSource(src);
    } 
    else if (auto filepath = hasOpt("-d", true)) {
        ifstream ifs(filepath);
        string src(std::istreambuf_iterator<char>(ifs), {});
        shell.fromSource(src, true);
    } else if (hasOpt("-d")) {
        string src(std::istreambuf_iterator<char>(cin), {});
        shell.fromSource(src, true);
    }
    else { // enter read-eval-print-loop
        shell.loop();
    }
}
