/*
 * Retrieve files via busybox wget.
 * Copyright 2000 Joey Hess <joeyh@debian.org>, GPL'd
 *
 * TODO: enable continuation if not going to stdout.
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "debconf.h"

#define DEBCONF_BASE "mirror/"

int main(int argc, char **argv) {
	int status;
	char *hostname, *directory, *dest, *url;
					    // TODO: what about ftp?
	debconf_command("GET", DEBCONF_BASE "http/hostname", NULL);
	hostname=strdup(debconf_ret());
	debconf_command("GET", DEBCONF_BASE "http/directory", NULL);
	directory=debconf_ret();
	
	if (argc < 3)
		dest="-"; /* stdout */
	else
		dest=argv[2];
	
	url=malloc( 7 /* http:// */ + strlen(hostname) + strlen(directory) +
		   1 /* / */ + strlen(argv[1]) + 1);
	sprintf(url, "http://%s%s/%s", hostname, directory, argv[1]);

	if (fork() == 0) {
		execlp("echo", "echo", "wget", url, "-O", dest, NULL);
		return 127;
	}
	else {
		wait(&status);
		return status;
	}
}
