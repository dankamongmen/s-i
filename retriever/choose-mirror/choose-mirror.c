/*
 * Mirror selection via debconf.
 * TODO: don't use DEBCONF_BASE like I do, that wastes space. Instead,
 *       make a function that appends it or something? Function shouldn't
 *       malloc... static buffer?
 */

#include <string.h>
#include <stdlib.h>
#include "mirrors.h"
#include "mirrors_http.h"
#include "debconf.h"

/*
 * Generates a list, suitable to be passed into debconf, from a
 * NULL-terminated string array.
 */
char *debconf_list(char *list[]) {
	int len, i, size = 1;
	char *ret = 0;
	
	for (i = 0; list[i] != NULL; i++) {
		len = strlen(list[i]);
		size += len;
		ret = realloc(ret, size + 2);
		memcpy(ret + size - len - 1, list[i], len);
		if (list[i+1] != NULL) {
			ret[size++ - 1] = ',';
			ret[size++ - 1] = ' ';
		}
	}

	return ret;
}

/* Returns an array of hostnames of mirrors in the specified country. */
char **mirrors_in(char *country) {
	char **ret=malloc(100 * sizeof(char *)); // TODO: don't hardcode size
	int i, j;
	for (i = j = 0; mirrors_http[i].country != NULL; i++)
		if (strcmp(mirrors_http[i].country, country) == 0)
			ret[j++]=mirrors_http[i].site;
	ret[j]=NULL;
	return ret;
}

/* Returns the root of the mirror, given the hostname. */
char *mirror_root(char *mirror) {
	int i;

	for (i = 0; mirrors_http[i].site != NULL; i++)
		if (strcmp(mirrors_http[i].site, mirror) == 0)
			return mirrors_http[i].root;

	return NULL;
}

int main (int argc, char **argv) {
	int manual_entry;
	char *list;
	char *mirror;
	int state = 1;
	
	debconf_command("CAPB", "backup", NULL);
	debconf_command("TITLE", "Choose mirror", NULL); //TODO: i18n
	
	/* Use a state machine to allow jumping back to previous questions. */
	while (state != 0 && state != 3) {
		switch (state) {
		case 1:
			/* Prompt for the country. */
			list=debconf_list(countries_http);
			debconf_command("SUBST", DEBCONF_BASE "country",
				"countries", list, NULL);
			free(list);
			debconf_command("INPUT", "high", DEBCONF_BASE "country", NULL);
			break;
		case 2:
			debconf_command("GET", DEBCONF_BASE "country", NULL);
			manual_entry = ! strcmp(debconf_ret(), "enter information manually");
			if (! manual_entry) {
				/* Prompt for mirror in selected country. */
				list=debconf_list(mirrors_in(debconf_ret()));
				debconf_command("SUBST",
					DEBCONF_BASE "http/mirror",
					"mirrors", list, NULL);
				debconf_command("INPUT", "medium",
					DEBCONF_BASE "http/mirror", NULL);
			}
			else {
				/* Manual entry. */
				debconf_command("INPUT", "critical",
						DEBCONF_BASE "http/hostname", NULL);
				debconf_command("INPUT", "critical",
						DEBCONF_BASE "http/directory", NULL);
			}
			/* Always ask about a proxy. */
			debconf_command("INPUT", "high", DEBCONF_BASE "http/proxy", NULL);
			break;
		case 3:
			if (! manual_entry) {
				/* 
				 * Copy information about the selected
				 * mirror into mirror/http/{hostname,directory},
				 * which is the standard location other
				 * tools can look at.
				 */
				debconf_command("GET", "http/mirror", NULL);
				mirror=debconf_ret();
				debconf_command("SET", "http/hostname", mirror, NULL);
				debconf_command("SET", "http/directory",
						mirror_root(mirror), NULL);
			}
			/* TODO: Check the mirror, and what else? See apt-setup */
			break;
		}

		if (debconf_command("GO", NULL) == 0)
			state++;
		else
			state--; /* back up */
	}

	if (state == 0)
		exit(10); /* backed all the way out */
	else
		exit(0);
}
