#include "main-menu.h"

#include <stdio.h>
#include <stdarg.h>

int debconf_command (const char *fmt, ...) {
	va_list ap;
	
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	/* TODO: read result from debconf */
	return 1;
}
