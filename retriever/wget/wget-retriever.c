/*
 * Retrieve files via busybox wget.
 * Copyright 2000 Joey Hess <joeyh@debian.org>, GPL'd
 */

#include <cdebconf/debconfclient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Base of the debconf question hierary used by this program. */
#define DEBCONF_BASE "mirror/"

int main(int argc, char **argv) {
	int ret;
	char *src;
	char *hostname, *directory, *command, *distribution;
	struct debconfclient *debconf = debconfclient_new();
	
	if (argc < 3)
		exit(1);
	
	/* Suck in data from the debconf database, which will be primed
	 * with the mirror to use by choose-mirror */
	// TODO: what about ftp?
	debconf->command(debconf, "GET", DEBCONF_BASE "distribution", NULL);
	distribution = strdup(debconf->value);
	debconf->command(debconf, "GET", DEBCONF_BASE "http/hostname", NULL);
	hostname = strdup(debconf->value);
	debconf->command(debconf, "GET", DEBCONF_BASE "http/directory", NULL);
	directory = strdup(debconf->value);
	debconf->command(debconf, "GET", DEBCONF_BASE "http/proxy", NULL);
	if (debconf->value && strcmp(debconf->value,"") != 0) {
		if (setenv("http_proxy", debconf->value, 1) == -1)
			exit(1);
	}
	src=argv[1];
	/*
	 * Intercept requests for a Packages file, and
	 * add the path to Packages files in the mirror.
	 * (All other files will be relative to the mirror 
	 * root directory.)
	 */
	if (strcmp(src, "Packages") == 0) {
                if (argc == 4) {
                        /* third argument is which suite we want */
                        asprintf(&src,"dists/%s/%s/debian-installer/binary-" 
                                 ARCH "/Packages", distribution, argv[3]);
                } else {
                        asprintf(&src,"dists/%s/main/debian-installer/binary-" 
                                 ARCH "/Packages", distribution);
                }
        }
	
	asprintf(&command, "wget -c -q http://%s%s/%s -O %s", hostname,
			directory, src, argv[2]);
        fprintf(stderr,"wget: %s\n", command);
	ret=system(command);
	if (ret == 256)
		return 1;
	return ret;
}
