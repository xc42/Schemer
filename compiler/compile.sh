#!/bin/zsh
output_path=../build/compiler
compiler=$output_path/sch-c

LD_LIBRARY_PATH=$output_path
export LD_LIBRARY_PATH

clang++ -L$output_path -lruntime -x assembler =(./transforms.rkt|$compiler|llc-14 --relocation-model=pic)
