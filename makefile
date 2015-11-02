#choose compiler
CC=gcc

#compile without linking
CFLAGS = -c

all: Shell

Shell: main.o
	$(CC) main.o -o Shell

main: main.c
	$(CC) main.c -o main.o
