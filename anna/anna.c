/*
 * anna's not nearly apt, but for the Debian installer, it will do. 
 *
 * anna is Copyright (C) 2000 by Joey Hess, under the terms of the GPL.
 * Apologetically dedicated to my sister, Anna.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "anna.h"

/*
 * This function takes a linked list of available packages, decides which
 * are worth installing, creates a linked list of those, and returns it.
 *
 * TODO: How it will eventually makes such choices is unknown, for now
 * it just returns all of the packages, putting the onus on whoever makes
 * the Packages file to make these decisions.
 *
 * TODO: one thing I know this needs to do is check for already-installed
 * packages and skip those.
 */
struct package_t *select_packages (struct package_t *packages) {
	return packages;
}

/* Calls udpkg to unpack a package. */
int unpack_package (char *pkgfile) {
	char *command=malloc(strlen(DPKG_UNPACK_COMMAND) + 1 +
			     strlen(pkgfile) + 1);
	sprintf(command, "%s %s", DPKG_UNPACK_COMMAND, pkgfile);
	return ! system(command);
}

/*
 * Retrives and unpacks each package in the linked list in turn.
 * 
 * Returns false on failure, and aborts the operation.
 */
int install_packages (struct package_t *packages) {
	struct package_t *p;
	char *f, *fp, *dest_file;
	
	for (p=packages; p; p=p->next) {
		if (p->filename) {
			/*
			 * Come up with a destination filename.. let's use
			 * the filename we're getting, minus any path
			 * component.
			 */
			for(f = fp = p->filename; *fp != 0; fp++)
				if (*fp == '/')
					f = ++fp;
			dest_file=malloc(strlen(DOWNLOAD_DIR) + 1 +
					 strlen(f) + 1);
			sprintf(dest_file, "%s/%s", DOWNLOAD_DIR, f);

			if (! get_package(p, dest_file)) {
				fprintf(stderr, "anna: error getting %s!",
					p->filename);
			}
			else if (! unpack_package(dest_file)) {
				unlink(dest_file);
				return(0);
			}
			unlink(dest_file);
			free(dest_file);
		}
	}

	return 1;
}

int main (int argc, char **argv) {
	/* Tell udpkg to shut up. */
	setenv("UDPKG_QUIET", "y", 1);
	
	return ! install_packages(select_packages(get_packages()));
}
