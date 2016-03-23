all : Socket Worker Manager 

CPPFLAGS = -g -I.
Socket : Socket.c
	gcc -c Socket.c

Manager : Manager.c
	gcc -std=c99 Socket.o Manager.c -o Manager

Worker : workerModificat.c
	gcc -std=c99 -pthread Socket.o  workerModificat.c -o workerM -I /usr/local/include/libxml2/ -lxml2

clean :
	rm *.o Manager workerM
