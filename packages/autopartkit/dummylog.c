#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "autopartkit.h"

void
autopartkit_error(int fatal, const char * format, ...)
{
    va_list ap;

    va_start(ap, format);
    fprintf(stderr, "error: ");
    vfprintf(stderr, format, ap);
    va_end(ap);
    if (fatal)
      exit(fatal);
}

void
autopartkit_log(const int level, const char * format, ...)
{
    va_list ap;

    fprintf(stderr, "log %d: ", level);

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

