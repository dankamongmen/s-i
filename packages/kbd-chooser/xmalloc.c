/* Error-free versions of some libc routines
 * We also move all printf-calling routines here; 
 * they are only called in the debug version; in other versions, we just die.
 *
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debian-installer.h>
#include "nls.h"

extern char *filename;
extern int line_nr;

static void
nomem(void) {
	di_error (": Out of memory\n");
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

/* fatal errors - change to varargs next time */
void
lkfatal(const char *s) {
	di_error (" %s:%d: %s\n", filename, line_nr, s);
	exit(1);
}

void
lkfatal0(const char *s, int d) {
	di_error( " %s:%d: ", filename, line_nr );
	di_error( s, d);
	exit(1);
}

void
lkfatal1(const char *s, const char *s2) {
	di_error("%s:%d: ", filename, line_nr);
	di_error( s, s2);
	exit(1);
}

