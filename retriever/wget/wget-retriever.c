/*
 * Retrieve files via busybox wget.
 * Copyright 2000 Joey Hess <joeyh@debian.org>, GPL'd
 *
 * TODO: enable continuation if not going to stdout.
 * TODO: proxy support
 * TODO: could stand to be a little more stable when it encounters NULLs.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "debconf.h"

#define DEBCONF_BASE "mirror/"

int main(int argc, char **argv) {
	int ret;
	char *hostname, *directory, *dest, *command;
					    // TODO: what about ftp?
	debconf_command("GET", DEBCONF_BASE "http/hostname", NULL);
	hostname=strdup(debconf_ret());
	debconf_command("GET", DEBCONF_BASE "http/directory", NULL);
	directory=debconf_ret();
	
	if (argc < 3)
		dest="-"; /* stdout */
	else
		dest=argv[2];
	
	command=malloc( 20 /* wget --quiet http:// */ + strlen(hostname) +
			strlen(directory) + 1 /* / */ + strlen(argv[1]) +
			4 /*  -O  */ + strlen(dest) + 1);
	sprintf(command, "wget --quiet http://%s%s/%s -O %s", hostname,
			directory, argv[1], dest);
	ret=system(command);
	if (ret == 256)
		return 1;
	return ret;
}
