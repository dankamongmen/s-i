
CC = gcc
CFLAGS = -Wall -O0 -g -ggdb
LD = gcc
LDFLAGS =

all: ppcdetect

ppcdetect: main.o
	${LD} ${LDFLAGS} -o ppcdetect $?

main.o: main.c main.h
	${CC} ${CFLAGS} -c main.c

clean:
	rm -f *.o
	rm -f core

distclean: clean
	rm -f ppcdetect

