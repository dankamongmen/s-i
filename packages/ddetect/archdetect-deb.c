#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "archdetect.h"

int main(int argc, char *argv[])
{
        const char *subarch;

	if (!(subarch = subarch_analyze()))
		return EXIT_FAILURE;

        printf("%s/%s\n", CPU_TEXT, subarch);

	return EXIT_SUCCESS;
}
