#!/bin/bash

maxthreads=$1

while :
do
    ./runTests.sh nosso outputs $1
done