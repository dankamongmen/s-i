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

/* fatal errors - change to varargs next time */
void
lkfatal(const char *s) {
	fprintf(stderr, "%s: %s:%d: %s\n", PROGNAME, filename, line_nr, s);
	exit(1);
}

void
lkfatal0(const char *s, int d) {
	fprintf(stderr, "%s: %s:%d: ", PROGNAME, filename, line_nr);
	fprintf(stderr, s, d);
	fprintf(stderr, "\n");
	exit(1);
}

void
lkfatal1(const char *s, const char *s2) {
	fprintf(stderr, "%s: %s:%d: ", PROGNAME, filename, line_nr);
	fprintf(stderr, s, s2);
	fprintf(stderr, "\n");
	exit(1);
}

