#include <string.h>

#include <cdebconf/debconfclient.h>

#include "archdetect.h"

int main(int argc, char *argv[])
{
        const char *subarch;

	if (!(subarch = subarch_analyze()))
		return 0;

	if (strstr(argv[0], "postinst") || (argc > 1 && strcmp(argv[1], "postinst") == 0)) {
                struct debconfclient *debconf;
		debconf = debconfclient_new();
		debconf_set(debconf, "debian-installer/kernel/subarchitecture", subarch);
	} else
		printf("%s/%s\n", CPU_TEXT, subarch);

	return 0;
}
