DEB_HOST_ARCH ?= $(shell dpkg-architecture -qDEB_HOST_ARCH)

CC = gcc
CFLAGS = -W -Wall -Os -DARCH_TEXT='"$(DEB_HOST_ARCH)"'
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

