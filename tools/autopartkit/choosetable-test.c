#include <stdio.h>
#include <stdarg.h>
#include "autopartkit.h"

void
autopartkit_log(const int level, const char * format, ...)
{
    va_list ap;

    fprintf(stderr, "log %d: ", level);

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

int main(int argc, char *argv[])
{
  int i;
  char *profiles[] = {
    NULL,
    "Server",
    "Server, Workstation",
    "Workstation",
    "Workstation, LTSP-server",
    "LTSP-server",
    "Server, LTSP-server",
    "Server, Workstation, LTSP-server"
  };

  for (i = 0; i < sizeof(profiles)/sizeof(profiles[0]); ++i) {
    printf("Test '%s' param: %s\n",
	   profiles[i] ? profiles[i] : "[null]",
	   choose_profile_table(profiles[i]));
  }
  return 0;
}
