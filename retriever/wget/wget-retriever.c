/*
 * Retrieve files via busybox wget.
 * Copyright 2000 Joey Hess <joeyh@debian.org>, GPL'd
 */

#include <cdebconf/debconfclient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBCONF_BASE "mirror/"

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
	debconf->command(debconf, "GET", DEBCONF_BASE "http/proxy", NULL);
	if (strcmp(debconf->value,"") == 0)
		if (setenv("http_proxy", debconf->value, 1) == -1)
			exit(1);
	
	src=argv[1];
	/*
	 * Intercept requests for a Packages file, and
	 * add the path to Packages files in the mirror.
	 * (All other files will be relative to the mirror 
	 * root directory.)
	 */
	if (strcmp(src, "Packages") == 0) {
		/* TODO: obviously this path is sid specific. FIXME */
		/* One way to fix this is choose-mirror could prompt for
		 * what version to install */
		src="dists/sid/main/debian-installer/binary-" ARCH "/Packages";
	}
	
	command=malloc( 18 /* wget -c -q http:// */ + strlen(hostname) +
			strlen(directory) + 1 /* / */ + strlen(src) +
			4 /*  -O  */ + strlen(argv[2]) + 1);
	sprintf(command, "wget -c -q http://%s%s/%s -O %s", hostname,
			directory, src, argv[2]);
	ret=system(command);
	if (ret == 256)
		return 1;
	return ret;
}
