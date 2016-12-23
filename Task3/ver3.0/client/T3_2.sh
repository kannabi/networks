#!/bin/bash

echo "Compiling"

gcc client.c -o transfer 

echo "Start"

##./transfer -f 111.jpg -h 10.5.10.70 -p 1529
##./transfer -f test.txt -h 10.5.10.70 -p 1529
./transfer -f ijvtest.txt -h 10.5.10.70 -p 1529
