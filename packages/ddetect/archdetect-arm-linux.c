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

struct map map_hardware[] = {
    { "Acorn-RiscPC" , "riscpc" },
    { "RiscStation-RS7500" , "riscstation" },
    { "Acorn-Archimedes" , "archimedies" },
    { "Acorn-A5000" , "a5000" },
    { "LinkUp Systems L7200" , "l7200" },
    { "FTV/PCI" , "futuretv" },
    { "unknown-TBOX" , "tbox" },
    { "CL-PS7110" , "ps7110" },
    { "Etoile" , "etoile" },
    { "LaCie_NAS" , "lacienas" },
    { "CL-PS7500" , "ps7500" },
    { "Anakin" , "anakin" },
    { "ATMEL AT91RM9200" , "at9200" },
    { "autronix autcpu12" , "clps711x" },
    { "Cirrus-CDB89712" , "clps711x" },
    { "Cirrus Logic 7212/7312" , "clps711x" },
    { "CL-EDB7211 (EP7211 eval board)" , "clps711x" },
    { "ARM-FortuNet" , "clps711x" },
    { "Guide A07 (cs89712 core)" , "clps711x" },
    { "ARM-Prospector720T" , "clps711x" },
    { "EBSA110" , "ebsa110" },
    { "Altera Epxa10db" , "expa" },
    { "Altera Epxa1db" , "expa" },
    { "EBSA285" , "netwinder" },
    { "Rebel-NetWinder" , "netwinder" },
    { "Chalice-CATS" , "netwinder" },
    { "co-EBSA285" , "netwinder" },
    { "Compaq-PersonalServer" , "netwinder" },
    { "ARM-Integrator" , "integrator" },
    { "Motorola MX1ADS" , "mx1ads" },
    { "ADS Advanced GraphicsClient" , "adsgraphics" },
    { "ADS Bitsy" , "adsbitsy" },
    { "ADS Bitsy Plus" , "adsbitsyplus" },
    { "Intel-Assabet" , "assabet" },
    { "Hewlett-Packard Laboratories BadgePAD 4" , "badgepad4" },
    { "Intel Brutus (SA1100 eval board)" , "brutus" },
    { "Iskratel Cep" , "cep" },
    { "Intrinsyc's Cerf Family of Products" , "intrinsyccerf" },
    { "empeg MP3 Car Audio Player" , "empeg" },
    { "FlexaNet" , "flexanet" },
    { "Freebird-HPC-1.1" , "hpc11" },
    { "2d3D, Inc. SA-1110 Development Board" , "2d3d" },
    { "ADS GraphicsClient" , "adsgraphicsclient" },
    { "ADS GraphicsMaster" , "adsgraphicsmaster" },
    { "Compaq iPAQ H3600" , "ipaqh3600" },
    { "Compaq iPAQ H3100" , "ipaqh3100" },
    { "Compaq iPAQ H3800" , "ipaqh3800" },
    { "HackKit Cpu Board" , "hackkit" },
    { "HuW-Webpanel" , "huwwebpanel" },
    { "Compaq Itsy" , "itsy" },
    { "HP Jornada 720" , "jornada720" },
    { "LART" , "lart" },
    { "BSE nanoEngine" , "bsenano" },
    { "OmniMeter" , "omnimeter" },
    { "Dialogue-Pangolin" , "pangolin" },
    { "Tulsa" , "tulsa" },
    { "PLEB" , "" },
    { "Shannon (AKA: Tuxscreen)" , "shannon" },
    { "Blazie Engineering Sherman" , "blaziesherman" },
    { "Simpad" , "simpad" },
    { "Picopeta-Simputer" , "simputer" },
    { "PT System 3" , "ptsystem3" },
    { "VisuAide Victor" , "victor" },
    { "XP860" , "xp860" },
    { "Yopy" , "yopy" },
    { "Shark" , "shark" },
    { "Simtec-BAST" , "bast" },
    { "Iyonix" , "iyonix" },
    { "Baloon" , "baloon" },
    { NULL, NULL }
};

const char *subarch_analyze(void)
{
	FILE *cpuinfo;
	char line[1024];
	char entry[256];
	char *pos;

	cpuinfo = fopen("/proc/cpuinfo", "r");
	if (cpuinfo == NULL)
		return "unknown";

	while (fgets(line, sizeof(line), cpuinfo) != NULL)
	{
	    if (strstr(line, "Hardware") == line)
	    {
	        pos = strchr(line, ':');
		if (pos == NULL)
			   continue;
		while (*++pos && isblank(*pos));

		strncpy(entry, pos, sizeof(entry));
		break;
	    }
	}

	fclose(cpuinfo);

	for (int i=0; map_hardware[i].entry; i++)
	{
	    if (!strncasecmp(map_hardware[i].entry, entry,
			strlen(map_hardware[i].entry)))
	    {
		return( map_hardware[i].ret );
	    }
	}

	return "unknown";
}
