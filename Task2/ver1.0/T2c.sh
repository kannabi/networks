#!/bin/bash

echo "Compiling"

gcc main.c server.c client.c header.h -o t2c.a 

echo "Start"

./t2c.a
