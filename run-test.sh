#!/bin/zsh

# configure and defs for interpreter
INTERPRETER=./build/interpreter/schemer

# configrue and defs for compiler
COMPILER_OUT_PATH=./build/compiler
FRONTPASS=./compiler/transforms.rkt
COMPILER=$COMPILER_OUT_PATH/sch-c
TESTS_FILE_DIR=./tests

LD_LIBRARY_PATH=$COMPILER_OUT_PATH
export LD_LIBRARY_PATH

function compiler_test() {
    total=0
    succ=0
    for testFile in $TESTS_FILE_DIR/*.scm; do 
        ((total=total+1))
        prog=`basename $testFile`.out
        clang++ -L$COMPILER_OUT_PATH -lruntime -x assembler -o $prog <($FRONTPASS < $testFile|$COMPILER|llc-14 --relocation-model=pic)
        myoutput=$(./$prog)
        stdoutput=`racket -e "$(<$testFile)"`
        if [ $myoutput = $stdoutput ]; then
            printf "test %s pass\n" $testFile
            ((succ=succ+1))
        else
            printf "test %s failed, expect %s got %s\n" $testFile $stdoutput $myoutput
        fi;
    done
    printf "Finished total %d compiler test run, %d passed\n" $total $succ
}

function interpreter_test() {
    total=0
    succ=0

    for testFile in $TESTS_FILE_DIR/*.scm; do 
        ((total=total+1))
        myoutput=$($INTERPRETER -e < $testFile)
        stdoutput=`racket -e "$(<$testFile)"`
        if [ $myoutput = $stdoutput ]; then
            printf "test %s pass\n" $testFile
            ((succ=succ+1))
        else
            printf "test %s failed, expect %s got %s\n" $testFile $stdoutput $myoutput
        fi;
    done
    printf "Finished total %d interpreter test run, %d passed\n" $total $succ
}

function main() {
    interpreter_test
    compiler_test
}


main

