# Makefile for echo Lab 

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: echoClient

echoClient: echoClient.o csapp.o
	$(CC) $(CFLAGS) echoClient.o csapp.o -o echoClient $(LDFLAGS)

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

echoClient.o: echoClient.c csapp.h
	$(CC) $(CFLAGS) -c echoClient.c

clean:
	rm -f *~ *.o echoClient