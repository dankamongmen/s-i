#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "archdetect.h"

struct map {
	char *model;
	char *subarch;
};

struct map map_model[] = {
	{ "Amiga", "amiga" },
	{ "Atari", "atari" },
	{ "Macintosh", "mac" },
	{ "BVME", "bvme6000" },
	{ "Motorola MVME147", "mvme147" },
	{ "Motorola", "mvme16x" },
	{ NULL, NULL }
};


const char *subarch_analyze(void)
{
	FILE *file;
	char line[1024];
	char model[256];
	char *pos;

	file = fopen("/proc/hardware", "r");
	if (file == NULL)
		return "unknown";

	while (fgets(line, sizeof(line), file) != NULL) 
	{
	    if (strstr(line, "Model:") == line)
	    {
	        pos = strchr(line, ':');
		if (pos == NULL)
			   continue;
		while (*++pos && isblank(*pos));

		strncpy(model, pos, sizeof(model));
		break;
	    }
	}

	fclose(file);

	for (int i=0; map_model[i].model; i++)
	{
	    if (!strncasecmp(map_model[i].model, model, 
			strlen(map_model[i].model)))
	    {
		return( map_model[i].subarch );
	    }
	}

	return "unknown";
}
