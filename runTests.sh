#!/bin/bash
mkdir -p $2
for i in $(seq 1 $3);
do
    for file in "$1"/*; 
    do
        echo "InputFile=$file NumThreads=$i"
        ./tecnicofs $file $2/"$(basename $file .txt)-$i".txt $i | grep -A1 "TecnicoFS completed in"
    done
done