#!/bin/zsh
output_path=../build/compiler
compiler=$output_path/sch-c

LD_LIBRARY_PATH=$output_path
export LD_LIBRARY_PATH

total=0
succ=0
for testFile in tests/*.scm; do
	((total=total+1))
	prog=`basename $testFile`.out
	clang++ -L$output_path -lruntime -x assembler -o $prog <(./transforms.rkt < $testFile|$compiler|llc-14)
	myoutput=$(./$prog)
	stdoutput=`racket -e "$(<$testFile)"`
	if [ $myoutput = $stdoutput ]; then
		printf "test %s pass\n" $testFile
		((succ=succ+1))
	else
		printf "test %s failed, expect %s got %s\n" $testFile $stdoutput $myoutput
	fi;
done

printf "Total %d test run, pass %d\n" $total $succ
