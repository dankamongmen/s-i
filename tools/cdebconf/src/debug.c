#ifndef NODEBUG
#include "common.h"
#include <stdio.h>

void debug_printf(int level, char *fmt, ...)
{
	static int loglevel = -1;
	va_list ap;
	if (loglevel < 0)
		loglevel = (getenv("DEBCONF_DEBUG") 
			? atoi(getenv("DEBCONF_DEBUG")) : 0);

	if (level <= loglevel)
	{
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}
#endif
