/*
 * Debconf communication routines.
 */

#include "main-menu.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/* Holds the textual return code of the last command. */
char *text;

/* Returns the last command's textual return code. */
char *debconf_ret (void) {
	return text;
}

/* Talks to debconf and returns the numeric return code. */
int debconf_command (const char *fmt, ...) {
	char buf[BUFSIZE];
	va_list ap;
	
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	fflush(stdout); /* make sure debconf sees it to prevent deadlock */

	fgets(buf, BUFSIZE, stdin);
	buf[strlen(buf)-1] = 0;
	if (strlen(buf)) {
		strtok(buf, " \t\n");
		text=strtok(NULL, "\n");
		return atoi(buf);
	}
	else {
		/* Nothing was entered; never really happens. */
		text=buf;
		return 0;
	}
}
