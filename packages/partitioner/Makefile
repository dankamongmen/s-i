
CC = gcc
CFLAGS = -W -Wall -Os
LD = gcc
LDFLAGS = -ldebconfclient -ldebian-installer -lparted -ldl

all: partitioner

partitioner: main.o
	${LD} ${LDFLAGS} -o partitioner $?

main.o: main.c
	${CC} ${CFLAGS} -c main.c

clean:
	rm -f *.o

distclean: clean
	rm -f partitioner

