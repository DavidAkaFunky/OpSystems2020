#!/bin/bash

# variables
inputdir=$1
outputdir=$2
maxthreads=$3

# argument validation
[ ! -d "$inputdir" ] && { echo "Input directory $inputdir doesn't exist. Please try again."; exit 1; }
[ ! -d "$outputdir" ] && { echo "Output directory $outputdir doesn't exist. Please try again."; exit 1; }
if [[ -n ${maxthreads//[0-9]/} ]]; then echo "Max number of threads must be numeric. Please try again."; exit 1; fi
[ ! $maxthreads -gt 0 ] && { echo "Max number of threads must be greater than 0. Please try again."; exit 1; }

for file in "$inputdir"/*; 
do
    for i in $(seq 1 $maxthreads);
    do
        echo "InputFile=$file NumThreads=$i"
        ./tecnicofs $file $outputdir/"$(basename $file .txt)-$i".txt $i | grep -A1 "TecnicoFS completed in"
    done
done