/* This is the codename of the preferred distribution; the one that the
 * current version of d-i is targeted at installing. */
#define PREFERRED_DISTRIBUTION "sarge"

#include <debian-installer.h>
#include <cdebconf/debconfclient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "mirrors.h"
#ifdef WITH_HTTP
#include "mirrors_http.h"
#endif
#ifdef WITH_FTP
#include "mirrors_ftp.h"
#endif
#if ! defined (WITH_HTTP) && ! defined (WITH_FTP)
#error Must compile with at least one of FTP or HTTP
#endif

static struct debconfclient *debconf;
static char *protocol = NULL;
static char *country  = NULL;

/*
 * Returns a string on the form "DEBCONF_BASE/protocol/supplied".  The
 * calling function is responsible for freeing the string afterwards.
 */
static char *add_protocol(char *string) {
	char *ret;

	assert(protocol != NULL); /* Fetched by choose_protocol */
	asprintf(&ret,DEBCONF_BASE "%s/%s",protocol,string);
	return ret;
}

/*
 * Generates a list, suitable to be passed into debconf, from a
 * NULL-terminated string array.
 */
static char *debconf_list(char *list[]) {
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
static struct mirror_t *mirror_list(void) {
	assert(protocol != NULL);

#ifdef WITH_HTTP
	if (strcasecmp(protocol,"http") == 0) {
		return mirrors_http;
	}
#endif
#ifdef WITH_FTP
	if (strcasecmp(protocol,"ftp") == 0) {
		return mirrors_ftp;
	}
#endif
	return 0; // should never happen
}

/* Returns an array of hostnames of mirrors in the specified country. */
static char **mirrors_in(char *country) {
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

static inline int has_mirror(char *country) {
	char **mirrors;
	mirrors = mirrors_in(country);
	return  (mirrors[0] == NULL) ? 0 : 1;
}

/* Returns the root of the mirror, given the hostname. */
static char *mirror_root(char *mirror) {
	int i;

	struct mirror_t *mirrors = mirror_list();
	
	for (i = 0; mirrors[i].site != NULL; i++)
		if (strcmp(mirrors[i].site, mirror) == 0)
			return mirrors[i].root;
	return NULL;
}

static int choose_country(void) {
	if (country)
		free(country);
	country = NULL;

	/* Pick a default country from elsewhere, eg countrychooser,*/
	if (debconf_get(debconf, DEBCONF_BASE "country") == 0) {
		// Not set yet. Seed with a default value
		if ((debconf_get(debconf, "debian-installer/country") == 0) &&
		    (debconf->value != NULL) ) {
				country = strdup (debconf->value);
				debconf_set (debconf, DEBCONF_BASE "country", country);
		}
	} else {
		country = debconf->value;
	}

	/* Ensure 'country' is set to something. */
	if (country == NULL || *country == 0) {
		country = "US";
	}

#ifdef WITH_HTTP
	if (strcasecmp(protocol,"http") == 0) {
		if (has_mirror(country)) {
			debconf_set(debconf, DEBCONF_BASE "http/countries", country);
		}
		debconf_input(debconf, "high", DEBCONF_BASE "http/countries");
	}
#endif
#ifdef WITH_FTP
	if (strcasecmp(protocol,"ftp") == 0) {
		if (has_mirror(country)) {
			debconf_set(debconf, DEBCONF_BASE "ftp/countries", country);
		}
 		debconf_input(debconf, "high", DEBCONF_BASE "ftp/countries");
	}
#endif
	
	return 0;
}

static int set_country(void) {
	debconf_get(debconf, (strcasecmp(protocol,"http") == 0 ) ? 
	            DEBCONF_BASE "http/countries" : DEBCONF_BASE "ftp/countries");
	country = strdup(debconf->value);
	debconf_set (debconf, DEBCONF_BASE "country", country);
	return 0;
}

static int choose_protocol(void) {
#if defined (WITH_HTTP) && defined (WITH_FTP)
	/* Both are supported, so ask. */
	debconf_subst(debconf, DEBCONF_BASE "protocol", "protocols", "http, ftp");
	debconf_input(debconf, "medium", DEBCONF_BASE "protocol");
#endif
	return 0;
}

static int get_protocol(void) {
#if defined (WITH_HTTP) && defined (WITH_FTP)
	debconf_get(debconf, DEBCONF_BASE "protocol");
	protocol = strdup(debconf->value);
#else
#ifdef WITH_HTTP
	debconf_set(debconf, DEBCONF_BASE "protocol", "http");
	protocol = "http";
#endif
#ifdef WITH_FTP
	debconf_set(debconf, DEBCONF_BASE "protocol", "ftp");
	protocol = "ftp";
#endif
#endif /* WITH_HTTP && WITH_FTP */
	return 0;
}

static int choose_suite(void) {
	debconf_input(debconf, "high", DEBCONF_BASE "suite");
	return 0;
}

static int manual_entry;

static int choose_mirror(void) {
	char *list;

	debconf_get(debconf, DEBCONF_BASE "country");
	manual_entry = ! strcmp(debconf->value, "enter information manually");
	if (! manual_entry) {
		char *mir = add_protocol("mirror");

                /* Prompt for mirror in selected country. */
		list=debconf_list(mirrors_in(country));
		debconf_subst(debconf, mir, "mirrors", list);
		free(list);
		
		debconf_input(debconf, "high", mir);
		free(mir);
	}
	else {
		char *host = add_protocol("hostname");
		char *dir = add_protocol("directory");

		/* Manual entry. */
		debconf_input(debconf, "critical", host);
		debconf_input(debconf, "critical", dir);
		
		free(host);
		free(dir);
	}

	return 0;
}

static int choose_proxy(void) {
	char *px;

	px = add_protocol("proxy");

	/* Always ask about a proxy. */
	debconf_input(debconf, "high", px);
	free(px);

	return 0;
}

static int validate_mirror(void) {
	char *mir;
	char *host;
	char *dir;
	char *prx;
	char *proxy;
	int ret = 0;

	mir = add_protocol("mirror");
	host = add_protocol("hostname");
	dir = add_protocol("directory");
	prx = add_protocol("proxy");
	
	debconf_get(debconf, prx);
	if (NULL != debconf->value)
		proxy = strdup(debconf->value);
	else
		proxy = NULL;
	free(prx);

	if (! manual_entry) {
		char *mirror;

		/*
		 * Copy information about the selected
		 * mirror into mirror/{protocol}/{hostname,directory},
		 * which is the standard location other
		 * tools can look at.
		 */
		debconf_get(debconf, mir);
		mirror = strdup(debconf->value);
		debconf_set(debconf, host, mirror);
		debconf_set(debconf, dir, mirror_root(mirror));
		free(mirror);
	} else {
		/* ret is 0 if everything is ok, 1 else, aka retval */
		/* Manual entry - check that the mirror is somewhat valid */
		debconf_get(debconf, host);
		if (debconf->value == NULL || strcmp(debconf->value,"") == 0 || strchr(debconf->value, '/') != NULL) {
			ret = 1;
		}
		debconf_get(debconf, dir);
		if (debconf->value == NULL || strcmp(debconf->value,"") == 0) {
			ret = 1;
		}
	}

	if (ret == 0) {
		/* Download and parse the Release file for the preferred
		 * distribution, to make sure that the mirror works, and to
		 * work out which suite it is currently in. Note that this
		 * assumes that mirrors w/o the preferred distribution are
		 * broken. */
		char *command;
		FILE *f = NULL;
		char *hostname, *directory;

		debconf_progress_start(debconf, 0, 1, DEBCONF_BASE "checking_title");
		debconf_progress_info(debconf, DEBCONF_BASE "checking_download");
		
		debconf_get(debconf, host);
		hostname = strdup(debconf->value);
		debconf_get(debconf, dir);
		directory = strdup(debconf->value);
	
		if ( NULL == proxy || (proxy && *proxy == '\0'))
			asprintf(&command, "wget -q %s://%s%s/%s/Release -O - | grep ^Suite: | cut -d' ' -f 2",
		                   protocol, hostname, directory,
				   "dists/" PREFERRED_DISTRIBUTION);
		else
			asprintf(&command, "%s_proxy=%s wget -q %s://%s%s/%s/Release -O - | grep ^Suite: | cut -d' ' -f 2",
		                   protocol, proxy, 
				   protocol, hostname, directory,
				   "dists/" PREFERRED_DISTRIBUTION);
		
		//fprintf(stderr, "command is %s\n", command);
		di_log(DI_LOG_LEVEL_DEBUG, "command: %s", command);
		
		free(hostname);
		free(directory);
		
		f = popen(command, "r");
		if (f != NULL) {
			char suite[32];
			if (fgets(suite, 31, f)) {
				if (suite[strlen(suite) - 1] == '\n')
					suite[strlen(suite) - 1] = '\0';
				debconf_set(debconf, DEBCONF_BASE "suite", suite);
			}
			else {
				ret = 1;
			}
			pclose(f);
		}
		else {
			ret = 1;
		}

		free(command);
		
		debconf_progress_step(debconf, 1);
		debconf_progress_stop(debconf);
	
		if (ret == 1) {
			debconf_input(debconf, "critical", DEBCONF_BASE "bad");
			if (debconf_go(debconf) == 30)
				exit(10); /* back up to menu */
		}
	}

	free(mir);
	free(host);
	free(dir);

	return ret;
}

int main (void) {
	/* Use a state machine with a function to run in each state */
	int state = 0;
	int (*states[])() = {
		choose_protocol,
		get_protocol,
		choose_country,
		set_country,
		choose_mirror,
		choose_proxy,
		validate_mirror,
                choose_suite,
		NULL,
	};

	debconf = debconfclient_new();
	debconf_capb(debconf, "backup");
	debconf_version(debconf, 2);

	di_system_init("choose-mirror");

	while (state >= 0 && states[state]) {
		if (states[state]() != 0) { /* back up to start */
			state = 0;
		}
		else if (debconf_go(debconf)) { /* back up */
			state = state - 1;
		}
		else  {
			state++;
		}
	}
	return (state >= 0) ? 0 : 10; /* backed all the way out */
}
