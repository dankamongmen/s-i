/*
 * Retrieve files via busybox wget.
 * Copyright 2000 Joey Hess <joeyh@debian.org>, GPL'd
 *
 * TODO: enable continuation if not going to stdout.
 * TODO: proxy support
 * TODO: could stand to be a little more stable when it encounters NULLs.
 */

#include <debconfclient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBCONF_BASE "mirror/"
#define DEMO 1

int main(int argc, char **argv) {
	int ret;
	char *src;
	char *hostname, *directory, *command;
	struct debconfclient *debconf = debconfclient_new();
	
	if (argc < 3)
		exit(1);
	
					    // TODO: what about ftp?
	debconf->command(debconf, "GET", DEBCONF_BASE "http/hostname", NULL);
	hostname=strdup(debconf->value);
	debconf->command(debconf, "GET", DEBCONF_BASE "http/directory", NULL);
	directory=debconf->value;
	
#ifdef DEMO
	/*
	 * For the demo system, ignore this, and use the only repository we 
	 * have.
	 */
	hostname="people.debian.org";
	directory="/~joeyh/debian-installer/udebs";
#endif
	
	src=argv[1];
	/*
	 * Intercept requests for a Packages file, and
	 * add the path to Packages files in the mirror.
	 * (All other files will be relative to the mirror 
	 * root directory.)
	 */
	if (strcmp(src, "Packages") == 0) {
#ifndef DEMO
		/* TODO: obviously this path is woody specific. FIXME */
		/*
		 * It's also probably not right, but we won't know until
		 * elmo gets off his ass.
		 */
		src="dists/woody/main/installer-" ARCH "/Packages";
#else
		/* For the demo system, I just hardcode this. */
		src="Packages";
#endif
	}
	
	command=malloc( 15 /* wget -q http:// */ + strlen(hostname) +
			strlen(directory) + 1 /* / */ + strlen(src) +
			4 /*  -O  */ + strlen(argv[2]) + 1);
	sprintf(command, "wget -q http://%s%s/%s -O %s", hostname,
			directory, src, argv[2]);
	ret=system(command);
	if (ret == 256)
		return 1;
	return ret;
}
