/*
 * Mirror selection via debconf.
 */

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
#error Must compile with at least one of FTP and HTTP
#endif

typedef enum { CM_NONE = -1, CM_PROTOCOL, CM_COUNTRY, CM_MIRROR, CM_PROXY, CM_VALIDATE, CM_DISTRIBUTION, CM_FINISHED } cm_state;

static cm_state last_asked = CM_NONE, asked = CM_NONE;

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

static inline int has_mirror(char *country)
{
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

	/* Pick a default country from elsewhere, eg languagechooser,*/
	if (debconf_get(debconf, DEBCONF_BASE "country") == 0) {
		// Not set yet. Seed with a default value
		if ((debconf_get(debconf, "debian-installer/country") == 0) &&
		    (debconf->value != NULL) ) {
				country = strdup (debconf->value);
				debconf_set (debconf, DEBCONF_BASE "country", country);
		}
	} else 
		country = debconf->value;

	// Ensure 'country' set to something
	if (country == NULL || *country == 0)
		country = "US";

#ifdef WITH_HTTP
	if (strcasecmp(protocol,"http") == 0) {
		if (has_mirror(country)) {
			debconf_set(debconf, DEBCONF_BASE "http/countries", country);
		}
		if (debconf_input(debconf, "high", DEBCONF_BASE "http/countries") == 0)
			asked =  CM_COUNTRY;
	}
#endif
#ifdef WITH_FTP
	if (strcasecmp(protocol,"ftp") == 0) {
		if (has_mirror(country)) {
			debconf_set(debconf, DEBCONF_BASE "ftp/countries", country);
		}
 		if (debconf_input(debconf, "high", DEBCONF_BASE "ftp/countries") == 0)
			asked = CM_COUNTRY;
	}
#endif

	if (debconf_go (debconf))
		return 30; // goback
	
	debconf_get (debconf,  (strcasecmp(protocol,"http") == 0 ) ? 
			DEBCONF_BASE "http/countries" : DEBCONF_BASE "ftp/countries");
	country = strdup(debconf->value);
	debconf_set (debconf, DEBCONF_BASE "country", country);
	return 0;
}

/** 
 * @brief   Choose which protocol to use.
 * @returns retcode from debconf asking the question
 */ 
static int choose_protocol(void) {
	int ret = 0;  
#if defined (WITH_HTTP) && defined (WITH_FTP)
	/* Both are supported, so ask. */
	debconf_subst(debconf, DEBCONF_BASE "protocol", "protocols", "http, ftp");
	if (debconf_input(debconf, "medium", DEBCONF_BASE "protocol") == 0)
		asked = CM_PROTOCOL;
	ret = debconf_go(debconf);
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
	
	return ret;
}

/* Choose which distribution to install. */
static int choose_distribution(void) {

	if (debconf_input(debconf, "high", DEBCONF_BASE "distribution"))
		asked = CM_DISTRIBUTION;
	return debconf_go (debconf);
}


static int manual_entry;

static int choose_mirror(void) {
	char *list;
	int ret;

	debconf_get(debconf, DEBCONF_BASE "country");
	manual_entry = ! strcmp(debconf->value, "enter information manually");
	if (! manual_entry) {
		char *mir = add_protocol("mirror");

                /* Prompt for mirror in selected country. */
		list=debconf_list(mirrors_in(country));
		debconf_subst(debconf, mir, "mirrors", list);
		free(list);
		
		if (debconf_input(debconf, "high", mir) == 0)
			asked = CM_MIRROR;
		free(mir);
		ret = debconf_go(debconf);
	} else {
		char *host = add_protocol("hostname");
		char *dir = add_protocol("directory");

		/* Manual entry. */
		asked = CM_MIRROR;
		while (1) {
			debconf_input(debconf, "critical", host);
			if (debconf_go(debconf) == 0) {
				debconf_input(debconf, "critical", dir);
				if (debconf_go(debconf) == 0) {
					ret = 0;
					break;
				}
			} else {
				ret = 30;
				break;
			}
		}
		free(host);
		free(dir);
	}
	return ret;
}

static int choose_proxy(void) {
	char *px;

	px = add_protocol("proxy");

	/* Always ask about a proxy. */
	if (debconf_input (debconf, "high", px) == 0)
		asked = CM_PROXY;
	free(px);
	return debconf_go(debconf);
}

static int validate_mirror(void) {
	char *mir;
	char *host;
	char *dir;
	int ret = 0;

	mir = add_protocol("mirror");
	host = add_protocol("hostname");
	dir = add_protocol("directory");

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
		if (debconf->value == NULL || strcmp(debconf->value,"") == 0) {
			debconf_fset(debconf, host, "seen", "false");
			ret = 1;
		}
		debconf_get(debconf, dir);
		if (debconf->value == NULL || strcmp(debconf->value,"") == 0) {
			debconf_fset(debconf, dir, "seen", "false");
			ret = 1;
		}
	}

	free(mir);
	free(host);
	free(dir);

	return ret;
}

int main (void) {
	/* Use a state machine with a function to run in each state */
	cm_state state = CM_PROTOCOL;
	int ret;
	int (*states[])() = {
		choose_protocol,
		choose_country,
		choose_mirror,
		choose_proxy,
		validate_mirror,
                choose_distribution,
		NULL,
	};

	debconf = debconfclient_new();
	debconf_capb(debconf, "backup");
	debconf_version(debconf, 2);

	last_asked = asked = CM_NONE;

	/*
	 * It's a pretty brain-dead state machine though. 
	 */
	while (state >= 0 && states[state]) {
		ret = states[state]();
		
		if (ret)  // goback signalled
			state = (asked == last_asked) ? last_asked -1 : last_asked; 
		else  {
			last_asked = asked;
			state++;
		}
	}
	return (state >= 0) ? 0 : 10 /* backed all the way out */;
}
