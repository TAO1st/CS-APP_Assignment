#! /bin/bash

echo -e "----------- Autograding Begin ------------"
for file in $(ls trace*)
do
    make test10 > tsh_$file
    make rtest10 > tshref_$file
    echo -e "Running and grading $file"
done 

for file in $(ls trace*)
do
    diff tsh_$file tshref_$file > diff_$file
done

echo -e "-------------------------------------\n"

for file in $(ls diff_trace*)
do
    echo $file " :"
    cat $file
    echo -e "\n-------------------------------------"
done