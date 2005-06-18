#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdetect.h"

int main(int argc, char *argv[])
{
        const char *subarch;

	if (!(subarch = subarch_analyze()))
 		return EXIT_FAILURE;

	printf("%s/%s\n", CPU_TEXT, subarch);

	return 0;
}
