#ifndef NODEBUG
#include "common.h"
#include <stdio.h>
#include <stdarg.h>

/*
 * Function: debug_printf
 * Input: int level - level of debug message
 *        char *fmt - format of message to write (a la printf)
 *        ... - other args for format string
 * Output: none
 * Description: prints a debug message to the screen if the level of the 
 *              message is higher than the current debug level
 * Assumptions: none
 */
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
