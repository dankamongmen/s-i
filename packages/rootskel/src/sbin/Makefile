dir = sbin

files_exec = \
	debian-installer \
	debian-installer-startup \
	shutdown \
	init \
	reopen-console \
	console-type \
	steal-ctty

console-type: console-type.c
	gcc -Os -Wall console-type.c -o console-type

steal-ctty: steal-ctty.c
	gcc -Os -Wall steal-ctty.c -o steal-ctty

build: console-type steal-ctty

clean:
	rm -f console-type steal-ctty

include ../../Makefile.inc
