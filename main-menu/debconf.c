#include "main-menu.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/* Holds the textual return code of the last command. */
char *text;

int debconf_command (const char *fmt, ...) {
	char buf[BUFSIZE];
	va_list ap;
	
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	fflush(stdout); /* make sure debconf sees it to prevent deadlock */

	fgets(buf, BUFSIZE, stdin);
	/* TODO: this could be more robust */
	strtok(buf, " \t\n");
	text=strtok(NULL, "\n");
	
	return atoi(buf);
}

char *debconf_ret (void) {
	return text;
}
