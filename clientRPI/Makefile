OBJS = stems.o clientRPI.o

CC = gcc
CFLAGS = -g -Wall

LIBS = -lpthread

.SUFFIXES: .c .o

clientRPI: clientRPI.o stems.o
	$(CC) $(CFLAGS) -o clientRPI clientRPI.o stems.o -lm $(LIBS) -lwiringPi

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clientRPI.o: stems.h

clean:
	-rm -f $(OBJS) clientRPI