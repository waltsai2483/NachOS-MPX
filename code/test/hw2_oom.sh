#!/bin/bash

testcases="oom"
pattern="There's no space for this program"

mkdir -p .tmp

../build.linux/nachos -e oom -ee -d u \
    > .tmp/"$testcases".txt 2>&1

grep "$pattern" .tmp/"$testcases".txt > /dev/null

if [ $? -eq 0 ]; then
    echo -e "\e[92m$testcases Succeed.\e[0m"
    rm -r .tmp
else
    echo -e "\e[91m$testcases Failed.\e[0m"
fi

