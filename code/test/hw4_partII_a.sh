#!/bin/sh
../build.linux/nachos -f
../build.linux/nachos -cp hw4_test1 /hw4_test1
../build.linux/nachos -e /hw4_test1
../build.linux/nachos -p /file1
../build.linux/nachos -cp hw4_test2 /hw4_test2
../build.linux/nachos -e /hw4_test2
