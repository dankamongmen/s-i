/*
 * Mirror selection via debconf.
 */

#include <cdebconf/debconfclient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mirrors.h"
#ifdef WITH_HTTP
#include "mirrors_http.h"
#endif
#ifdef WITH_FTP
#include "mirrors_ftp.h"
#endif
#if ! defined (WITH_HTTP) && ! defined (WITH_FTP)
#error Must compile with at least one of FTP and HTTP
#endif

struct debconfclient *debconf;

/*
 * Returns a string on the form "DEBCONF_BASE/protocol/supplied".  The
 * calling function is responsible for freeing the string afterwards.
 */

char *add_protocol(char *string) {
	char *protocol;
	char *ret;

	debconf->command(debconf, "GET", DEBCONF_BASE "protocol", NULL);
	protocol = strdup(debconf->value);
	
	asprintf(&ret,"%s%s/%s",DEBCONF_BASE,protocol,string);
	free(protocol);
	return ret;
}

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
		ret[size -1] = '\0';
	}
	return ret;
}

/*
 * Returns the correct mirror list, depending on whether protocol is
 * set to http or ftp.  Do NOT free the structure - it is a pointer to
 * the static list in mirrors_protocol.h
 */

struct mirror_t *mirror_list(void) {
	debconf->command(debconf, "GET", DEBCONF_BASE "protocol", NULL);

#ifdef WITH_HTTP
	if (strcasecmp(debconf->value,"http") == 0) {
		return mirrors_http;
	}
#endif
#ifdef WITH_FTP
	if (strcasecmp(debconf->value,"ftp") == 0) {
		return mirrors_ftp;
	}
#endif
	return 0; // should never happen
}

/* Returns an array of hostnames of mirrors in the specified country. */
char **mirrors_in(char *country) {
        static char **ret;
	int i, j, num = 1;
	struct mirror_t *mirrors = mirror_list();

	ret = malloc(num * sizeof(char *));
	for (i = j = 0; mirrors[i].country != NULL; i++) {
		if (j == num-1) {
			num *= 2;
			ret = realloc(ret,num * sizeof(char*));
		}
	if (strcmp(mirrors[i].country, country) == 0) {
			ret[j++]=mirrors[i].site;
		}
	}
	ret[j]=NULL;
	return ret;
}

/* Returns the root of the mirror, given the hostname. */
char *mirror_root(char *mirror) {
	int i;

	struct mirror_t *mirrors = mirror_list();
	
	for (i = 0; mirrors[i].site != NULL; i++)
		if (strcmp(mirrors[i].site, mirror) == 0)
			return mirrors[i].root;

	return NULL;
}

int choose_country(void) {
	char *list = 0;
	debconf->command(debconf, "GET", DEBCONF_BASE "protocol", NULL);
#ifdef WITH_HTTP
	if (debconf->value != NULL && strcasecmp(debconf->value,"http") == 0) {
		list = debconf_list(countries_http);
	}
#endif
#ifdef WITH_FTP
	if (debconf->value != NULL && strcasecmp(debconf->value,"ftp") == 0) {
		list = debconf_list(countries_ftp);
	}
#endif

	debconf->command(debconf, "SUBST", DEBCONF_BASE "country", "countries", list, NULL);
	free(list);
	debconf->command(debconf, "INPUT", "high", DEBCONF_BASE "country", NULL);
	return 0;
}

/* Choose which protocol to use. */
int choose_protocol(void) {
#if defined (WITH_HTTP) && defined (WITH_FTP)
	/* Both are supported, so ask. */
	debconf->command(debconf, "SUBST", DEBCONF_BASE "protocol", "protocols", "http, ftp", NULL);
	debconf->command(debconf, "INPUT", "high", DEBCONF_BASE "protocol", NULL);
#else
#ifdef WITH_HTTP
	debconf->command(debconf, "SET", DEBCONF_BASE "protocol", "http");
#endif
#ifdef WITH_FTP
	debconf->command(debconf, "SET", DEBCONF_BASE "protocol", "ftp");
#endif
#endif /* WITH_HTTP && WITH_FTP */
	return 0;
}

/* Choose which distribution to install. */
int choose_distribution(void) {
	debconf->command(debconf, "INPUT", "high", DEBCONF_BASE "distribution", NULL);
        return 0;
}


int manual_entry;

int choose_mirror(void) {
	char *list;
	char *protocol, *country;
	debconf->command(debconf, "GET", DEBCONF_BASE "protocol", NULL);
	protocol = strdup(debconf->value);
	
	debconf->command(debconf, "GET", DEBCONF_BASE "country", NULL);
	country = strdup(debconf->value);
	manual_entry = ! strcmp(debconf->value, "enter information manually");
	if (! manual_entry) {
                /* Prompt for mirror in selected country. */

		list=debconf_list(mirrors_in(country));

		debconf->command(debconf, "SUBST", add_protocol("mirror"), "mirrors", list, NULL);
		free(list);
		debconf->command(debconf, "INPUT", "medium", add_protocol("mirror"), NULL);
		
	}
	else {
		/* Manual entry. */
		debconf->command(debconf, "INPUT", "critical", add_protocol("hostname"), NULL);
		debconf->command(debconf, "INPUT", "critical", add_protocol("directory"), NULL);
	}
	/* Always ask about a proxy. */
	debconf->command(debconf, "INPUT", "high", add_protocol("proxy"), NULL);
	free(protocol);
	free(country);
	return 0;
}

int validate_mirror(void) {
	char *mirror;
	char *protocol;

	debconf->command(debconf, "GET", DEBCONF_BASE "protocol", NULL);
	protocol = strdup(debconf->value);

	if (! manual_entry) {
		/*
		 * Copy information about the selected
		 * mirror into mirror/{protocol}/{hostname,directory},
		 * which is the standard location other
		 * tools can look at.
		 */
		debconf->command(debconf, "GET", add_protocol("mirror"), NULL);
		mirror=strdup(debconf->value);
		debconf->command(debconf, "SET", add_protocol("hostname"), 
				 mirror, NULL);
		debconf->command(debconf, "SET", add_protocol("directory"),
				 mirror_root(mirror), NULL);
		free(mirror);
		return 0;
	} else {
		int not_ok = 0; /* Is 0 if everything is ok, 1 else, aka retval */
		/* Manual entry - check that the mirror is somewhat valid */
		debconf->command(debconf, "GET", add_protocol("hostname"), NULL);		
		if (debconf->value == NULL || strcmp(debconf->value,"") == 0) {
			debconf->command(debconf, "fset", add_protocol("hostname"), "seen", "false", NULL);
			not_ok = 1;
		}
		debconf->command(debconf, "GET", add_protocol("directory"), NULL);		
		if (debconf->value == NULL || strcmp(debconf->value,"") == 0) {
			debconf->command(debconf, "fset", add_protocol("directory"), "seen", "false", NULL);
			not_ok = 1;
		}
		return not_ok;
	}
	return 0;
}

int main (int argc, char **argv) {
	/* Use a state machine with a function to run in each state */
	int state = 0;
	int ret;
	int (*states[])() = {
		choose_protocol,
		choose_country,
		choose_mirror,
		validate_mirror,
                choose_distribution,
		NULL,
	};

	debconf = debconfclient_new();
	debconf->command(debconf, "CAPB", "backup", NULL);
	debconf->command(debconf, "TITLE", "Choose mirror", NULL); //TODO: i18n

	/*
	 * It's a pretty brain-dead state machine though. It advances
	 * forward and back by one state always. Enough for our purposes.
	 */
	while (state >= 0 && states[state]) {
		ret = states[state]();
		
		if (ret == 0 && debconf->command(debconf, "GO", NULL) == 0) 
			state++;
		else
			state--; /* back up */
	}

	if (state >= 0)
		exit(0);
	else
		exit(10); /* backed all the way out */
}
