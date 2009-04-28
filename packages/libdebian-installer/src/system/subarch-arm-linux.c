#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <debian-installer/system/subarch.h>

struct map {
	char *entry;
	char *ret;
};

static struct map map_hardware[] = {
    { "Acorn-RiscPC" , "rpc" },
    { "EBSA285" , "netwinder" },
    { "Rebel-NetWinder" , "netwinder" },
    { "Chalice-CATS" , "netwinder" },
    { "co-EBSA285" , "netwinder" },
    { "Compaq-PersonalServer" , "netwinder" },
    { "Freescale MX51 Babbage Board", "imx51" }, /* iMX51 reference hardware. */
    { "ADS" , "ads" }, /* Matches only ADS boards. Put any exceptions before. */
    { "Applied Data Systems" , "ads" }, /* More ADS boards. */
    { "Marvell DB-88F6281-BP Development Board", "kirkwood" },
    { "Marvell RD-88F6192-NAS Development Board", "kirkwood" },
    { "Marvell RD-88F6281 Reference Board", "kirkwood" },
    { "Marvell SheevaPlug Reference Board", "kirkwood" },
    { "QNAP TS-119/TS-219", "kirkwood" },
    { "Buffalo/Revogear Kurobox Pro", "orion5x" },
    { "D-Link DNS-323", "orion5x" },
    { "QNAP TS-109/TS-209", "orion5x" },
    { "QNAP TS-409", "orion5x" },
    { "HP Media Vault mv2120", "orion5x" },
    { "Buffalo Linkstation Pro/Live", "orion5x" },
    { "Marvell Orion-NAS Reference Design", "orion5x" },
    { "Marvell Orion-2 Development Board", "orion5x" },
    { "Intel EP80219", "iop32x" },
    { "Intel IQ31244", "iop32x" },
    { "Intel IQ80321", "iop32x" },
    { "Thecus N2100", "iop32x" },
    { "Thecus N4100", "iop32x" },
    { "GLAN Tank", "iop32x" },
    { "Intel IQ80331", "iop33x" },
    { "Intel IQ80332", "iop33x" },
    { "ADI Engineering Coyote", "ixp4xx" },
    { "Freecom Storage Gateway", "ixp4xx" },
    { "Intel IXDPG425", "ixp4xx" },
    { "Intel IXDP425 Development Platform", "ixp4xx" },
    { "Intel IXDP465 Development Platform", "ixp4xx" },
    { "Intel IXCDP1100 Development Platform", "ixp4xx" },
    { "Gateworks Avila Network Platform", "ixp4xx" },
    { "Gemtek GTWX5715 (Linksys WRV54G)", "ixp4xx" },
    { "Iomega NAS 100d", "ixp4xx" },
    { "Linksys NSLU2", "ixp4xx" },
    { "ARM-Versatile AB", "versatile" },
    { "ARM-Versatile PB", "versatile" },
    { NULL, NULL }
};

const char *di_system_subarch_analyze(void)
{
	FILE *cpuinfo;
	char line[1024];
	char entry[256];
	char *pos;
	int i;

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
		while (*++pos && (*pos == '\t' || *pos == ' '));

		strncpy(entry, pos, sizeof(entry));
		break;
	    }
	}

	fclose(cpuinfo);

	for (i = 0; map_hardware[i].entry; i++)
	{
	    if (!strncasecmp(map_hardware[i].entry, entry,
			strlen(map_hardware[i].entry)))
	    {
		return( map_hardware[i].ret );
	    }
	}

	return "unknown";
}
