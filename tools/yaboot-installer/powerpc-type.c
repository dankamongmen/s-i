/*
  Determine what PowerPC subarchitecture we're running.
  Copyright (C) 2003 Thorsten Sauter <tsauter@gmx.net>
  Copyright (C) 2002 Colin Walters <walters@gnu.org>
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "powerpc-type.h"

int get_powerpc_type(void) {
	int status;
	status = system("test -f /proc/cpuinfo && grep -q \"pmac-generation.*NewWorld\" /proc/cpuinfo");

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		return(0);
	}

	return(1);
}

#ifdef POWERPC_TYPE_MAIN

int main(int argc, char **argv) {
	if(get_powerpc_type() == 0) {
		printf("NewWorld PowerMac\n");
	} else {
		printf("Unknown\n");
	}

	exit(0);
}

#endif

