#!/bin/bash

echo "Compiling"

gcc main.c -o L1 -lm

echo "Start"

./L1 -p 2048

