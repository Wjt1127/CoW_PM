CC = gcc
CFLAGS = -Wall -g -I..
PROGS = test

all: $(PROGS)

test: test.c
	$(CC) $(CFLAGS) -lpthread -o $@ $^