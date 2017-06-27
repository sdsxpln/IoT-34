#
# To compile, type "make" or make "all"
# To remove files, type "make clean"
#
OBJS = server.o request.o stems.o clientGet.o clientPost.o pushServer.o pushClient.o
TARGET = server pushServer

CC = gcc
CFLAGS = -g -Wall

LIBS = -lpthread 

.SUFFIXES: .c .o 

all: server clientPost clientGet dataGet.cgi dataPost.cgi pushServer pushClient push.cgi

server: server.o request.o stems.o
	$(CC) $(CFLAGS) -o server server.o request.o stems.o $(LIBS)

clientGet: clientGet.o stems.o
	$(CC) $(CFLAGS) -o clientGet clientGet.o stems.o

clientPost: clientPost.o stems.o
	$(CC) $(CFLAGS) -o clientPost clientPost.o stems.o -lm $(LIBS) 

dataGet.cgi: dataGet.c stems.h
	$(CC) $(CFLAGS) -o dataGet.cgi dataGet.c -lmysqlclient

dataPost.cgi: dataPost.o stems.o
	$(CC) $(CFLAGS) -o dataPost.cgi dataPost.o stems.o -lmysqlclient

pushServer: pushServer.o request.o stems.o
	$(CC) $(CFLAGS) -o pushServer pushServer.o request.o stems.o $(LIBS)

pushClient: pushClient.o stems.o
	$(CC) $(CFLAGS) -o pushClient pushClient.o stems.o

push.cgi: push.o stems.o
	$(CC) $(CFLAGS) -o push.cgi push.o stems.o

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

server.o: stems.h request.h
clientGet.o: stems.h
clientPost.o: stems.h
dataPost.o: stems.h
server.o: stems.h request.h
serverClient: stems.h
push.o: stems.h

clean:
	-rm -f $(OBJS) server clientPost clientGet dataGet.cgi dataPost.cgi pushServer pushClient push.cgi
