
CC = gcc
CFLAGS = -Wall -O0 -g -ggdb
LD = gcc
LDFLAGS = -ldebconf -ldebian-installer -lparted -ldl

all: partitioner

partitioner: main.o
	${LD} ${LDFLAGS} -o partitioner $?

main.o: main.c main.h
	${CC} ${CFLAGS} -c main.c

clean:
	rm -f *.o

distclean: clean
	rm -f partitioner

