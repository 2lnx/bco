#! /usr/bin/sh

CC=gcc

main: m.o
    gcc m.o -o main

m.o: m.c
    $(CC) -c m.c -o m.o
