#! /bin/bash

echo -e "----------- Autograding Begin ------------"
for i in {01..16}
do
    make test$i > tsh_trace$i.txt
    make rtest$i > tshref_trace$i.txt
    echo -e "Running and grading trace$i.txt"
done 

for file in $(ls trace*)
do
    diff -l "./sdriver.pl -t trace" tsh_$file tshref_$file > diff_$file -I "./sdriver.pl -t trace"
done

echo -e "-------------------------------------\n"

for file in $(ls diff_trace*)
do
    echo $file " :"
    cat $file
    echo -e "\n-------------------------------------"
done