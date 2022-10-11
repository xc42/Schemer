#! /bin/zsh

total=0
succ=0

for f in tests/*; do
	stdOut=`racket -e "$(cat $f)"`
	transOut=`racket -e "$(./transforms.rkt < $f)"`

	((total=total+1))
	if [ $stdOut = $transOut ]; then
		((succ=succ+1))
	else
		printf "Test fail at file: %s\n" $f
	fi;
done

printf "Total %d test(s) run, %d test(s) pass\n" $total $succ
