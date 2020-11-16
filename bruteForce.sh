#!/bin/bash

maxthreads=$1

while :
do
    ./runTests.sh inputs outputs $1
done