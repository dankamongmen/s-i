/*
 * dpkg status file reading routines
 */

#include "main-menu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <search.h>
#include <locale.h>

/*
 * Read status file and generate and return a linked list of packages.
 */
struct linkedlist_t *status_read(void) {
	FILE *f;
	struct linkedlist_t *plist;

	if ((f = fopen(STATUSFILE, "r")) == NULL) {
		perror(STATUSFILE);
		return 0;
	}
        plist = di_pkg_parse(f);
        fclose(f);
	di_pkg_resolve_deps(plist);

	return plist;
}
