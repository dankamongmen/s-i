/* Error-free versions of some libc routines */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debian-installer.h>
#include "nls.h"

static void
nomem(void) {
	di_log (PROGNAME ": Out of memory\n");
	exit(1);
}


void *
xmalloc(size_t sz) {
	void *p = malloc(sz);
	if (p == NULL)
		nomem();
	return p;
}

void *
xrealloc(void *pp, size_t sz) {
	void *p = realloc(pp, sz);
	if (p == NULL)
		nomem();
	return p;
}

char *
xstrdup(char *p) {
	char *q = strdup(p);
	if (q == NULL)
		nomem();
	return q;
}


