#include "main-menu.h"

#include <stdio.h>

int debconf_command (char *command) {
	printf("%s\n", command);
	/* TODO: read result from debconf */
	return 1;
}
