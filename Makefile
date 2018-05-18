# Makefile


## CC  = Compiler
## CFLAGS = Compiler flags
CC	= gcc
CFLAGS =	-Wall -Wextra -std=gnu99 -pthread


## OBJ = Object files
## SRC = Source files
## EXE = Executable name

SRC =		HTTPServer.c
OBJ =		HTTPServer.o
EXE = 	server

## Top level target is executable
$(EXE):	$(OBJ)
		$(CC) $(CFLAGS) -o $(EXE) $(OBJ)

## Clean: Remove object files and core dump files
clean:
		/bin/rm $(OBJ)

## Clobber: Performs Clean and removes executable file

clobber: clean
		/bin/rm $(EXE)

## Dependencies
HTTPServer.o:	HTTPServer.h
