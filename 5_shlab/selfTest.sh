#! /bin/bash

echo -e "----------- Autograding Begin ------------"
for i in {01..16}
do
    ./sdriver.pl -t trace$i.txt -s ./tshref -a -p > tshref_trace$i.txt
    ./sdriver.pl -t trace$i.txt -s ./tsh -a -p > tsh_trace$i.txt
    echo -e "Running and grading trace$i.txt"
done 

for file in $(ls trace*)
do
    diff tsh_$file tshref_$file > diff_$file -I "./sdriver.pl -t trace"
done

echo -e "-------------------------------------\n"

for file in $(ls diff_trace*)
do
    echo $file " :"
    cat $file
    echo -e "\n-------------------------------------"
done