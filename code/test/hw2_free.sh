#!/bin/bash

testcases="free_addrspace"
pattern="Release pages of the addrspace."

mkdir -p .tmp

../build.linux/nachos -e consoleIO_test1 -e consoleIO_test2 -ee -d u \
    > .tmp/"$testcases".txt 2>&1

grep "$pattern" .tmp/"$testcases".txt > /dev/null

if [ $? -eq 0 ]; then
    echo -e "\e[92m$testcases Succeed.\e[0m"
    rm -r .tmp
else
    echo -e "\e[91m$testcases Failed.\e[0m"
fi

