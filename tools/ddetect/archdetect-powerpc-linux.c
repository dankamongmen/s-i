#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "archdetect.h"

struct map
{
  char *entry;
  char *ret;
};

struct map map_generation[] =
{
  { "OldWorld", "powermac_oldworld" },
  { "NewWorld", "powermac_newworld" },
  { NULL, NULL }
};

struct map map_machine[] =
{
  { "PReP", "prep" },
  { "CHRP Pegasos", "chrp_pegasos" },
  { "CHRP", "chrp" },
  { "Amiga", "amiga" },
  { NULL, NULL }
};

char *check_map (struct map map[], const char *entry)
{
  for (; map->entry; map++)
    if (!strncasecmp (map->entry, entry, strlen (map->entry)))
      return map->ret;
  return NULL;
}

const char *subarch_analyze() {
	FILE *cpuinfo;
	char line[1024];
        char cpuinfo_machine[256], cpuinfo_generation[256];
        char *ret, *pos;

	cpuinfo = fopen("/proc/cpuinfo", "r");
	if(cpuinfo == NULL)
          return "unknown";

	while(fgets(line, sizeof (line), cpuinfo) != NULL) {
		if(line == NULL)
			continue;
		pos = strchr(line, ':');
		if(pos == NULL)
			continue;
		while(*++pos && isblank(*pos));

		if(strstr(line, "machine") == line)
			strncpy(cpuinfo_machine, pos, sizeof (cpuinfo_machine));

		if(strstr(line, "pmac-generation") == line)
			strncpy(cpuinfo_generation, pos, sizeof (cpuinfo_generation));
	}

	fclose(cpuinfo);

        ret = check_map (map_machine, cpuinfo_machine);
        if (ret)
          return ret;
        ret = check_map (map_generation, cpuinfo_generation);
        if (ret)
          return ret;

        return "unknown";
}

