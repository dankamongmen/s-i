#ifndef NODEBUG
#include "common.h"
#include <stdio.h>
#include <stdarg.h>
#ifdef SYSLOG_LOGGING
#include <syslog.h>
#endif

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
#ifndef SYSLOG_LOGGING
		va_start(ap, fmt);
		vfprintf(fp, fmt, ap);
		va_end(ap);

		fflush(fp);
#else
		va_start(ap, fmt);
		vsyslog(LOG_USER | LOG_DEBUG, fmt, ap);
		va_end(ap);
#endif
	}
}
#endif /* DEBUG */
