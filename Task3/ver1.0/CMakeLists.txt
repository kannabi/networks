cmake_minimum_required(VERSION 3.6)
project(Task2)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(SOURCE_FILES main.с server.c client.c header.h)
add_executable(Task2 ${SOURCE_FILES})