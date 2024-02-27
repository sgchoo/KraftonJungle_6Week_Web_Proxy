# Makefile for echo Lab 

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: echoServer

echoServer: echoServer.o csapp.o
	$(CC) $(CFLAGS) echoServer.o csapp.o -o echoServer $(LDFLAGS)

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

echoServer.o: echoServer.c csapp.h
	$(CC) $(CFLAGS) -c echoServer.c

clean:
	rm -f *~ *.o echoServer