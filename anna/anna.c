/*
 * anna's not nearly apt, but for the Debian installer, it will do. 
 *
 * anna is Copyright (C) 2000 by Joey Hess, under the terms of the GPL.
 * Apologetically dedicated to my sister, Anna.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
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
        struct package_t *p = packages;
        struct package_t *prev = NULL;
        while (p)
        {
                if (strcmp(p->package,"cdebconf-udeb") == 0 ||
                    strcmp(p->package,"anna") == 0 ||
                    strcmp(p->package,"rootskel") == 0 )
                {
                        if (prev)
                                prev->next = p->next;
                        else
                                packages = p->next;
                        p = p->next;
                        continue;
                }
                prev = p;
                p = p->next;
        }
	return packages;
}

/* Calls udpkg to unpack a package. */
int unpack_package (char *pkgfile) {
	char *command=malloc(strlen(DPKG_UNPACK_COMMAND) + 1 +
			     strlen(pkgfile) + 1);
	sprintf(command, "%s %s", DPKG_UNPACK_COMMAND, pkgfile);
	return ! system(command);
}

/* check whether the md5sum of file matches sum.  if they don't,
 * return 0.
 */

int md5sum(char* sum, char *file) {
        int io[2];
        int pid;
        char line[1024];
        pipe(io);
        pid = fork();
        if (pid == 0) {
                /* child */
                dup2(io[1],1);
                execl("/usr/bin/md5sum","/usr/bin/md5sum",file,0);
        }
        wait(NULL);
        read(io[0],&line,1023);
        line[1023] = '\0';
        if (strlen(line) < 32) {
                /* not a success, return */
                return 0;
        }
        line[32] = '\0';
        /* line now contains just the md5sum */
        return ! strcmp(line,sum);
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
				fprintf(stderr, "anna: error getting %s!\n",
					p->filename);
				exit(1);
			} else if (! md5sum(p->md5sum, dest_file)) {
                          fprintf(stderr, "anna: md5sum mismatch on %s!\n",
                                  p->filename);
                          unlink(dest_file);
                          exit(1);
                        } else if (! unpack_package(dest_file)) {
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
