dir =

subdirs_empty = \
	dev \
	initrd \
	lib \
	mnt \
	proc \
	sys

files_exec = \
	init

build: init

init: init.c
	klcc -Os -Wall -shared init.c -o init

clean:
	rm -f init

include ../Makefile.inc
