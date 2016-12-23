#!/bin/bash

echo "Compiling"

gcc main.c server.c client.c header.h -o t2.a 

echo "Start"

./t2.a
