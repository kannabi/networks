#!/bin/bash

echo "Compiling"

gcc main.c server.c client.c  -o t2.a

echo "Start"

./t2.a
