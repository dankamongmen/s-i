/*
 * Retrieve files via busybox wget.
 * Copyright 2000 Joey Hess <joeyh@debian.org>, GPL'd
 */

#include <cdebconf/debconfclient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Base of the debconf question hierarchy used by this program. */
#define DEBCONF_BASE "mirror/"

/*
 * Get a protocol question value from the debconf mirror hierarchy.
 * Returns the result in the value field of debconfclient.
 */
static void get_mirror_protocol_value(struct debconfclient *debconf,
		char *protocol, char *string)
{
	char *question;

	asprintf(&question, DEBCONF_BASE "%s/%s", protocol, string);
	debconf->command(debconf, "GET", question, NULL);
	free(question);
}

int main(int argc, char **argv) {
	int ret;
	char *src;
	char *hostname, *directory, *command;
	struct debconfclient *debconf = debconfclient_new();

	if (argc < 2)
		exit(1);
	if (strcmp(argv[1], "retrieve") == 0) {
		char *protocol;

		if (argc < 4)
			exit(1);

		debconf->command(debconf, "GET", DEBCONF_BASE "protocol", NULL);
		protocol = strdup(debconf->value);

		/* Suck in data from the debconf database, which will be primed
		 * with the mirror to use by choose-mirror */
		get_mirror_protocol_value(debconf, protocol, "hostname");
		hostname = strdup(debconf->value);

		get_mirror_protocol_value(debconf, protocol, "directory");
		directory = strdup(debconf->value);

		get_mirror_protocol_value(debconf, protocol, "proxy");
		if (debconf->value && strcmp(debconf->value,"") != 0) {
			char *env_variable;

			asprintf(&env_variable, "%s_proxy", protocol);
			if (setenv(env_variable, debconf->value, 1) == -1)
				exit(1);
			free(env_variable);
		}
		src = argv[2];
		asprintf(&command, "wget -c -q %s://%s%s/%s -O %s", protocol,
			 hostname, directory, src, argv[3]);
		ret = system(command);
		if (ret == 256)
			return 1;
		return ret;
	} else {
		/* unknown command */
		return 1;
	}
}

