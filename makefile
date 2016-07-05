#! /usr/bin/sh

CC=gcc
CFLAGS=-Wall -O -g

OBJ=m.o

main:$(OBJ)
    $(CC) $^ -o %@
    
%.o:%c
    $(CC) -c $< -o $@

clean:
	rm *.o
