#!/bin/bash

# variables
inputdir=$1
outputdir=$2
maxthreads=$3

# argument validation
[ ! -d "$inputdir" ] && { echo "Input directory $inputdir doesn't exist. Please try again."; exit 1; }
[ ! -d "$outputdir" ] && { echo "Output directory $outputdir doesn't exist. Please try again."; exit 1; }
if ! [[ $yournumber =~ $re ]] ; then echo "Max number of threads must be... oh well... a number. Please try again."; exit 1; fi
[ ! $maxthreads -gt 0 ] && { echo "Max number of threads must be greater than 0. Please try again."; exit 1; }

for i in $(seq 1 $maxthreads);
do
    for file in "$inputdir"/*; 
    do
        echo "InputFile=$file NumThreads=$i"
        ./tecnicofs $file $outputdir/"$(basename $file .txt)-$i".txt $i | grep -A1 "TecnicoFS completed in"
    done
done