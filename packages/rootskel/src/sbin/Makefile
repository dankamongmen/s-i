dir = sbin

files_exec = \
	debian-installer \
	debian-installer-startup \
	shutdown \
	init \
	reopen-console \
	steal-ctty

steal-ctty: steal-ctty.c
	gcc -Os -Wall steal-ctty.c -o steal-ctty

build: steal-ctty

clean:
	rm -f steal-ctty

include ../../Makefile.inc
