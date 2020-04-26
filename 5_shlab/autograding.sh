#! /bin/bash

echo -e "----------- \033[42;37mAutograding Begin\033[0m ------------"
for i in {01..16}
do
    ./sdriver.pl -t trace$i.txt -s ./tsh -a -p > tsh_trace$i.txt
    ./sdriver.pl -t trace$i.txt -s ./tshref -a -p > tshref_trace$i.txt
    echo -e "Running and grading trace$i.txt (5 points)"
done 

for file in $(ls trace*)
do
    diff tsh_$file tshref_$file > diff_$file -I "./sdriver.pl -t trace"
done

echo -e "------------ \033[42;37mShow diff begin\033[0m -------------"

for file in $(ls diff_trace*)
do
    echo -e "\033[33m$file : \033[0m "
    cat $file
    echo -e "\n-------------------------------------------------"
done

echo -e "\033[33mYour score will be computed out of a maximum of 90 points based on the following distribution:\033[0m"
echo -e "\033[33m80 Correctness: 16 trace files at 5 points each.\033[0m"
echo -e "\033[33m10 Style points. We expect you to have good comments (5 pts) and to check the return value of EVERY system call (5 pts).\033[0m\n"