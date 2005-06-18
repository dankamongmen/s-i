#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "archdetect.h"

struct map {
	char *entry;
	char *ret;
};

struct map map_generation[] = {
	{ "OldWorld", "powermac_oldworld" },
	{ "NewWorld", "powermac_newworld" },
	{ "NuBus", "powermac_nubus" },
	{ NULL, NULL }
};

struct map map_machine[] = {
	{ "PReP", "prep" },
	{ "CHRP Pegasos", "chrp_pegasos" },
	{ "CHRP IBM,7028-6C4", "chrp_rs6k" },
	{ "CHRP IBM,7029-6E3", "chrp_rs6k" },
	{ "CHRP IBM,7038-6M2", "chrp_rs6k" },
	{ "CHRP IBM,7039-651", "chrp_rs6k" },
	{ "CHRP IBM,7040-681", "chrp_rs6k" },
	{ "CHRP IBM,7044-270", "chrp_rs6k" },
	{ "CHRP IBM,7046-B50", "chrp_rs6k" },
	{ "CHRP IBM,8842-21X", "chrp_rs6k" },
	{ "CHRP IBM,9111-520", "chrp_rs6k" },
	{ "CHRP IBM,9113-550", "chrp_rs6k" },
	{ "CHRP IBM,9117-570", "chrp_rs6k" },
	{ "CHRP IBM,9124-720", "chrp_rs6k" },
	{ "CHRP", "chrp" },
	{ "Amiga", "amiga" },
	{ "64-bit iSeries Logical Partition", "iseries" },
	{ NULL, NULL }
};

static char *check_map(struct map map[], const char *entry)
{
	for (; map->entry; map++)
		if (!strncasecmp(map->entry, entry, strlen(map->entry)))
			return map->ret;

	return NULL;
}

const char *subarch_analyze(void)
{
	FILE *cpuinfo;
	char line[1024];
	char cpuinfo_machine[256], cpuinfo_generation[256];
	char *ret, *pos;

	cpuinfo = fopen("/proc/cpuinfo", "r");
	if (cpuinfo == NULL)
		return "unknown";

	while (fgets(line, sizeof(line), cpuinfo) != NULL) {
		pos = strchr(line, ':');
		if (pos == NULL)
			continue;
		while (*++pos && (*pos == '\t' || *pos == ' '));

		if (strstr(line, "machine") == line)
			strncpy(cpuinfo_machine, pos, sizeof(cpuinfo_machine));

		if (strstr(line, "pmac-generation") == line)
			strncpy(cpuinfo_generation, pos, sizeof(cpuinfo_generation));
	}

	fclose(cpuinfo);

	ret = check_map(map_machine, cpuinfo_machine);
	if (ret)
		return ret;
	ret = check_map(map_generation, cpuinfo_generation);
	if (ret)
		return ret;

	return "unknown";
}
