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
 * Assumptions: "remembers" the debug log level and log file
 */
void debug_printf(int level, char *fmt, ...)
{
	static int loglevel = -1;
	static FILE *fp = NULL;
	va_list ap;
	if (loglevel < 0)
	{
		loglevel = (getenv("DEBCONF_DEBUG") 
			? atoi(getenv("DEBCONF_DEBUG")) : 0);

		if (getenv("DEBCONF_DEBUGFILE") == NULL ||
		    (fp = fopen(getenv("DEBCONF_DEBUGFILE"), "a")) == NULL)
			fp = stderr;
	}

	if (level <= loglevel)
	{
		va_start(ap, fmt);
		vfprintf(fp, fmt, ap);
		va_end(ap);

		fflush(fp);
	}
}
#endif
