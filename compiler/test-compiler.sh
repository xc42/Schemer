#!/bin/zsh 

compiler=../build/compiler/sch-c

for $f in `ls tests`; do
	ans=$(racket -e 
