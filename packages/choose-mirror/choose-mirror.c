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
#if defined (WITH_FTP) && defined (WITH_FTP_MANUAL)
#error Only one of WITH_FTP and WITH_FTP_MANUAL can be defined
#endif

static struct debconfclient *debconf;
static char *protocol = NULL;
static char *country  = NULL;
int show_progress = 1;

/* Are we installing from a CD that includes base system packages? */
static int base_on_cd = 0;

/* Available releases (suite/codename) on the mirror. */
static struct release_t releases[MAXRELEASES];

/*
 * Returns a string on the form "DEBCONF_BASE/protocol/supplied". The
 * calling function is responsible for freeing the string afterwards.
 */
static char *add_protocol(char *string) {
	char *ret;

	assert(protocol != NULL); /* Fetched by choose_protocol */
	asprintf(&ret, DEBCONF_BASE "%s/%s", protocol, string);
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
 * set to http or ftp. Do NOT free the structure - it is a pointer to
 * the static list in mirrors_protocol.h
 */
static struct mirror_t *mirror_list(void) {
	assert(protocol != NULL);

#ifdef WITH_HTTP
	if (strcasecmp(protocol, "http") == 0)
		return mirrors_http;
#endif
#ifdef WITH_FTP
	if (strcasecmp(protocol, "ftp") == 0)
		return mirrors_ftp;
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
		if (j == num - 1) {
			num *= 2;
			ret = realloc(ret, num * sizeof(char*));
		}
		if (strcmp(mirrors[i].country, country) == 0)
			ret[j++] = mirrors[i].site;
	}
	ret[j] = NULL;
	return ret;
}

/* returns true if there is a mirror in the specified country */
static inline int has_mirror(char *country) {
	char **mirrors;
	if (strcmp(country, MANUAL_ENTRY) == 0)
		return 1;
	mirrors = mirrors_in(country);
	return (mirrors[0] == NULL) ? 0 : 1;
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

/*
 * Get the default suite (can be a codename) to use; this is either a
 * preseeded value or a value set at build time.
 */
static char *get_default_suite(void) {
	char *suite = NULL;
	FILE *f = NULL;
	char buf[SUITE_LENGTH];

	/* Check for a preseeded suite/codename. */
	debconf_get(debconf, DEBCONF_BASE "suite");
	if (! base_on_cd && strlen(debconf->value) > 0) {
		suite = strdup(debconf->value);
	} else {
		/* Check for default suite/codename set at build time. */
		f = fopen("/etc/default-release", "r");
		if (f != NULL) {
			if (fgets(buf, SUITE_LENGTH - 1, f)) {
				if (buf[strlen(buf) - 1] == '\n')
					buf[strlen(buf) - 1] = '\0';
				suite = strdup(buf);
			}
			fclose(f);
		}
	}
	return suite;
}

void log_invalid_release(const char *name, const char *field) {
	di_log(DI_LOG_LEVEL_WARNING,
		"broken mirror: invalid %s in Release file for %s", field, name);
}

static int get_release(struct release_t *release, const char *name);

/*
 * Try to fetch a Release file using its codename; if successful, check
 * that it matches the Release file that was fetched using the suite.
 * Returns false only if an invalid Release file was found.
 */
static int validate_codename(struct release_t *s_release) {
	struct release_t cn_release;
	int ret = 1;

	memset(&cn_release, 0, sizeof(struct release_t));

	/* s_release->name is the codename to check */
	if (get_release(&cn_release, s_release->name)) {
		if ((cn_release.status & IS_VALID) &&
		    strcmp(cn_release.suite, s_release->suite) == 0) {
			s_release->status |= (cn_release.status & GET_CODENAME);
		} else {
			s_release->status &= ~IS_VALID;
			ret = 0;
		}
	}

	free(cn_release.name);
	free(cn_release.suite);

	return ret;
}

/*
 * Fetch a Release file, extract its Suite and Codename and check its valitity.
 */
static int get_release(struct release_t *release, const char *name) {
	char *command;
	FILE *f = NULL;
	char *hostname, *directory;
	char line[80];
	char buf[SUITE_LENGTH];

	hostname = add_protocol("hostname");
	debconf_get(debconf, hostname);
	free(hostname);
	hostname = strdup(debconf->value);
	directory = add_protocol("directory");
	debconf_get(debconf, directory);
	free(directory);
	directory = strdup(debconf->value);

	asprintf(&command, "wget -q %s://%s%s/dists/%s/Release -O - | grep -E '^(Suite|Codename):'",
		 protocol, hostname, directory, name);
	di_log(DI_LOG_LEVEL_DEBUG, "command: %s", command);
	f = popen(command, "r");
	free(command);
	free(hostname);
	free(directory);

	if (f != NULL) {
		while (fgets(line, sizeof(line), f) != NULL) {
			char *value;

			if (line[strlen(line) - 1] == '\n')
				line[strlen(line) - 1] = '\0';
			if ((value = strstr(line, ": ")) != NULL) {
				strncpy(buf, value + 2, SUITE_LENGTH - 1);
				buf[SUITE_LENGTH - 1] = '\0';
				if (strncmp(line, "Codename:", 9) == 0)
					release->name = strdup(buf);
				if (strncmp(line, "Suite:", 6) == 0)
					release->suite = strdup(buf);
			}
		}
		if (release->name != NULL && strcmp(release->name, name) == 0)
			release->status |= IS_VALID | GET_CODENAME;
		if (release->suite != NULL && strcmp(release->suite, name) == 0)
			release->status |= IS_VALID | GET_SUITE;

		if ((release->name != NULL || release->suite != NULL) &&
		    !(release->status & IS_VALID))
			log_invalid_release(name, "Suite or Codename");

		/* Check if release can also be gotten using codename */
		if ((release->status & IS_VALID) && release->name != NULL &&
		    !(release->status & GET_CODENAME))
			if (! validate_codename(release))
				log_invalid_release(name, "Codename");

		/* In case there is no Codename field */
		if ((release->status & IS_VALID) && release->name == NULL)
			release->name = strdup(name);

		// di_log(DI_LOG_LEVEL_DEBUG, "get_release(): %s -> %s:%s (0x%x)",
		//	name, release->suite, release->name, release->status);
	}

	pclose(f);

	if (release->name != NULL) {
		return 1;
	} else {
		if (release->suite)
			free(release->suite);
		return 0;
	}
}

static int find_releases(void) {
	int nbr_suites = sizeof(suites)/SUITE_LENGTH;
	int i, r = 0, bad_mirror = 0;
	struct release_t release;
	char *default_suite;

	default_suite = get_default_suite();
	if (default_suite == NULL)
		di_log(DI_LOG_LEVEL_ERROR, "no default release specified");

	if (show_progress) {
		debconf_progress_start(debconf, 0, nbr_suites,
				       DEBCONF_BASE "checking_title");
		debconf_progress_info(debconf,
				      DEBCONF_BASE "checking_download");
	}

	/* Initialize releases; also ensures NULL termination for .name */
	memset(&releases, 0, MAXRELEASES * sizeof(struct release_t));

	/* Get releases for all suites */
	if (! base_on_cd) {
		for (i=0; i < nbr_suites && r < MAXRELEASES; i++) {
			memset(&release, 0, sizeof(struct release_t));
			if (get_release(&release, suites[i])) {
				if (release.status & IS_VALID) {
					if (strcmp(release.name, default_suite) == 0 ||
					    strcmp(release.suite, default_suite) == 0)
						release.status |= IS_DEFAULT;
					releases[r++] = release;
				} else {
					bad_mirror = 1;
					break;
				}
			}

			if (show_progress)
				debconf_progress_step(debconf, 1);
		}
		if (r == MAXRELEASES)
			di_log(DI_LOG_LEVEL_ERROR, "array overflow: more releases than allowed by MAXRELEASES");
		if (! bad_mirror && r == 0)
			di_log(DI_LOG_LEVEL_INFO, "mirror does not have any suite symlinks");
	}

	/* Try to get release using the default "suite" */
	if (! bad_mirror && (base_on_cd || r == 0)) {
		memset(&release, 0, sizeof(struct release_t));
		if (get_release(&release, default_suite)) {
			if (release.status & IS_VALID) {
				release.status |= IS_DEFAULT;
				releases[r++] = release;
			} else {
				bad_mirror = 1;
			}
		}
	}

	if (show_progress) {
		debconf_progress_step(debconf, nbr_suites);
		debconf_progress_stop(debconf);
	}

	free(default_suite);

	if (r == 0 || bad_mirror) {
		if (release.name)
			free(release.name);
		if (release.suite)
			free(release.suite);

		debconf_input(debconf, "critical", DEBCONF_BASE "bad");
		if (debconf_go(debconf) == 30)
			exit(10); /* back up to menu */
		else
			return 1; /* back to beginning of questions */
	}

	return 0;
}

/*
 * Using the current debconf settings for a mirror, figure out which suite
 * to use from the mirror and set mirror/suite.
 *
 * This is accomplished by downloading the Release file from the mirror.
 * Suite selection tries each suite in turn, and stops at the first one that
 * seems usable.
 *
 * If no Release file is found, returns false. That probably means the
 * mirror is broken or unreachable.
 */
int find_suite (void) {
	char *command;
	FILE *f = NULL;
	char *hostname, *directory;
	int nbr_suites = sizeof(suites)/SUITE_LENGTH;
	int i;
	int ret = 0;
	char buf[SUITE_LENGTH];

	if (show_progress) {
		debconf_progress_start(debconf, 0, 1,
				       DEBCONF_BASE "checking_title");
		debconf_progress_info(debconf,
				      DEBCONF_BASE "checking_download");
	}

	hostname = add_protocol("hostname");
	debconf_get(debconf, hostname);
	free(hostname);
	hostname = strdup(debconf->value);
	directory = add_protocol("directory");
	debconf_get(debconf, directory);
	free(directory);
	directory = strdup(debconf->value);

	/* Try each suite in turn until one is found that works. */
	for (i=0; i <= nbr_suites && ! ret; i++) {
		char *suite;

		if (i == 0) {
			/* First check for a (preseeded) default suite. */
			suite = get_default_suite();
			if (suite == NULL)
				continue;
		} else {
			suite = strdup(suites[i - 1]);
		}

		asprintf(&command, "wget -q %s://%s%s/dists/%s/Release -O - | grep ^Suite: | cut -d' ' -f 2",
			 protocol, hostname, directory, suite);
		di_log(DI_LOG_LEVEL_DEBUG, "command: %s", command);
		f = popen(command, "r");
		free(command);

		if (f != NULL) {
			if (fgets(buf, SUITE_LENGTH - 1, f)) {
				if (buf[strlen(buf) - 1] == '\n')
					buf[strlen(buf) - 1] = '\0';
				debconf_set(debconf, DEBCONF_BASE "suite", buf);
				ret = 1;
			}
		}

		pclose(f);
		free(suite);
	}

	free(hostname);
	free(directory);

	if (show_progress) {
		debconf_progress_step(debconf, 1);
		debconf_progress_stop(debconf);
	}

	return ret;
}

static int check_base_on_cd(void) {
	FILE *fp;
	if ((fp = fopen("/cdrom/.disk/base_installable", "r"))) {
		base_on_cd = 1;
		fclose(fp);
	}
	return 0;
}

static int choose_protocol(void) {
#if defined (WITH_HTTP) && (defined (WITH_FTP) || defined (WITH_FTP_MANUAL))
	/* Both are supported, so ask. */
	debconf_subst(debconf, DEBCONF_BASE "protocol", "protocols", "http, ftp");
	debconf_input(debconf, "medium", DEBCONF_BASE "protocol");
#endif
	return 0;
}

static int get_protocol(void) {
#if defined (WITH_HTTP) && (defined (WITH_FTP) || defined (WITH_FTP_MANUAL))
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

static int choose_country(void) {
	if (country)
		free(country);
	country = NULL;

#if defined (WITH_FTP_MANUAL)
	assert(protocol != NULL);
	if (strcasecmp(protocol, "ftp") == 0)
		return 0;
#endif

	debconf_get(debconf, DEBCONF_BASE "country");
	if (! strlen(debconf->value)) {
		/* Not set yet. Seed with a default value. */
		if ((debconf_get(debconf, "debian-installer/country") == 0) &&
		    (debconf->value != NULL) ) {
			country = strdup (debconf->value);
			debconf_set(debconf, DEBCONF_BASE "country", country);
		}
	} else {
		country = debconf->value;
	}

	/* Ensure 'country' is set to something. */
	if (country == NULL || *country == 0)
		country = "US";

	char *countries;
	countries = add_protocol("countries");
	if (has_mirror(country)) {
		debconf_set(debconf, countries, country);
		debconf_fget(debconf, DEBCONF_BASE "country", "seen");
		debconf_fset(debconf, countries, "seen", debconf->value);
	}
	debconf_input(debconf, "high", countries);

	free (countries);
	return 0;
}

static int set_country(void) {
	char *countries;

#if defined (WITH_FTP_MANUAL)
	assert(protocol != NULL);
	if (strcasecmp(protocol, "ftp") == 0)
		return 0;
#endif

	countries = add_protocol("countries");
	debconf_get(debconf, countries);
	country = strdup(debconf->value);
	debconf_set(debconf, DEBCONF_BASE "country", country);

	free (countries);
	return 0;
}

static int manual_entry;

static int choose_mirror(void) {
	char *list;

	debconf_get(debconf, DEBCONF_BASE "country");
#ifndef WITH_FTP_MANUAL
	manual_entry = ! strcmp(debconf->value, MANUAL_ENTRY);
#else
	if (! strcasecmp(protocol, "ftp") == 0)
		manual_entry = ! strcmp(debconf->value, MANUAL_ENTRY);
	else
		manual_entry = 1;
#endif

	if (! manual_entry) {
		char *mir = add_protocol("mirror");

		/* Prompt for mirror in selected country. */
		list = debconf_list(mirrors_in(country));
		debconf_subst(debconf, mir, "mirrors", list);
		free(list);

		debconf_input(debconf, "high", mir);
		free(mir);
	} else {
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

/* Check basic validity of the selected/entered mirror. */
static int validate_mirror(void) {
	char *mir;
	char *host;
	char *dir;
	int valid = 1;

	mir = add_protocol("mirror");
	host = add_protocol("hostname");
	dir = add_protocol("directory");

	if (! manual_entry) {
		char *mirror;
		char *root;

		/*
		 * Copy information about the selected
		 * mirror into mirror/{protocol}/{hostname,directory},
		 * which is the standard location other
		 * tools can look at.
		 */
		debconf_get(debconf, mir);
		mirror = strdup(debconf->value);
		debconf_set(debconf, host, mirror);
		root = mirror_root(mirror);
		free(mirror);
		if (root == NULL)
			valid = 0;
		else
			debconf_set(debconf, dir, root);
	} else {
		/* check if manually entered mirror is basically ok */
		debconf_get(debconf, host);
		if (debconf->value == NULL || strcmp(debconf->value, "") == 0 ||
		    strchr(debconf->value, '/') != NULL)
			valid = 0;
		debconf_get(debconf, dir);
		if (debconf->value == NULL || strcmp(debconf->value, "") == 0)
			valid = 0;
	}

	free(mir);
	free(host);
	free(dir);

	if (valid) {
		return 0;
	} else {
		/* ToDo: Could use a more appropriate error message */
		debconf_input(debconf, "critical", DEBCONF_BASE "bad");
		if (debconf_go(debconf) == 30)
			exit(10); /* back up to menu */
		else
			return 9; /* back up to choose_mirror */
	}
}

static int choose_proxy(void) {
	char *px = add_protocol("proxy");

	/* Always ask about a proxy. */
	debconf_input(debconf, "high", px);

	free(px);
	return 0;
}

static int set_proxy(void) {
	char *px = add_protocol("proxy");
	char *proxy_var;

	asprintf(&proxy_var, "%s_proxy", protocol);

	debconf_get(debconf, px);
	if (debconf->value != NULL && strlen(debconf->value)) {
		if (strchr(debconf->value, ':')) {
			setenv(proxy_var, debconf->value, 1);
		} else {
			char *proxy_value;
			asprintf(&proxy_value, "http://%s", debconf->value);
			setenv(proxy_var, proxy_value, 1);
			free(proxy_value);
		}
	} else {
		unsetenv(proxy_var);
	}

	free(proxy_var);
	free(px);

	return 0;
}

static int check_mirror(void) {
	if (find_suite()) {
		return 0;
	} else {
		debconf_input(debconf, "critical", DEBCONF_BASE "bad");
		if (debconf_go(debconf) == 30)
			exit(10); /* back up to menu */
		else
			return 1; /* back to beginning of questions */
	}
}

static int choose_suite(void) {
	char *choices_c[MAXRELEASES], *choices[MAXRELEASES], *list;
	int i, ret;

	ret = find_releases();
	if (ret)
		return ret;

	/* Also ensures NULL termination */
	memset(choices, 0, MAXRELEASES * sizeof(char *));
	memset(choices_c, 0, MAXRELEASES * sizeof(char *));

	/* Arrays can never overflow as we've already checked releases */
	for (i=0; releases[i].name != NULL; i++) {
		char *name;

		if (releases[i].status & GET_SUITE)
			name = strdup(releases[i].suite);
		else
			name = strdup(releases[i].name);

		choices_c[i] = strdup(name);
		if (strcmp(name, releases[i].name) != 0)
			asprintf(&choices[i], "%s - %s", releases[i].name, name);
		else
			choices[i] = strdup(name);
		if (releases[i].status & IS_DEFAULT)
			debconf_set(debconf, DEBCONF_BASE "suite", name);

		free(name);
	}

	list = debconf_list(choices_c);
	debconf_subst(debconf, DEBCONF_BASE "suite", "CHOICES-C", list);
	free(list);
	list = debconf_list(choices);
	debconf_subst(debconf, DEBCONF_BASE "suite", "CHOICES", list);
	free(list);

	for (i=0; choices[i] != NULL; i++) {
		free(choices_c[i]);
		free(choices[i]);
	}

	/* If the base system can be installed from CD, don't allow to
	 * select a different suite
	 */
	if (! base_on_cd)
		debconf_input(debconf, "medium", DEBCONF_BASE "suite");

	return 0;
}

/* Set the codename for the selected suite. */
int set_codename (void) {
	char *suite;
	int i;

	/* As suite has been determined previously, this should not fail */
	debconf_get(debconf, DEBCONF_BASE "suite");
	if (strlen(debconf->value) > 0) {
		suite = strdup(debconf->value);
		di_log(DI_LOG_LEVEL_INFO, "selected release ('suite'): %s", suite);

		for (i=0; releases[i].name != NULL; i++) {
			if (strcmp(releases[i].name, suite) == 0 ||
			    strcmp(releases[i].suite, suite) == 0) {
				char *codename;

				if (releases[i].status & GET_CODENAME)
					codename = releases[i].name;
				else
					codename = releases[i].suite;
				debconf_set(debconf, DEBCONF_BASE "codename", codename);
				di_log(DI_LOG_LEVEL_INFO, "codename set to: %s", codename);
				break;
			}
		}

		free(suite);
	}

	return 0;
}

/* Check if the mirror carries the architecture that's being installed. */
int check_arch (void) {
	char *command;
	FILE *f = NULL;
	char *hostname, *directory, *suite = NULL;
	int valid = 0;

	hostname = add_protocol("hostname");
	debconf_get(debconf, hostname);
	free(hostname);
	hostname = strdup(debconf->value);
	directory = add_protocol("directory");
	debconf_get(debconf, directory);
	free(directory);
	directory = strdup(debconf->value);

	/* As suite has been determined previously, this should not fail */
	debconf_get(debconf, DEBCONF_BASE "suite");
	if (strlen(debconf->value) > 0) {
		suite = strdup(debconf->value);

		asprintf(&command, "wget -q %s://%s%s/dists/%s/main/binary-%s/Release -O - | grep ^Architecture:",
			 protocol, hostname, directory, suite, ARCH_TEXT);
		di_log(DI_LOG_LEVEL_DEBUG, "command: %s", command);
		f = popen(command, "r");
		free(command);

		if (f != NULL) {
			char buf[SUITE_LENGTH];
			if (fgets(buf, SUITE_LENGTH - 1, f))
				if (strlen(buf) > 1)
					valid = 1;
		}
		pclose(f);
	}

	free(hostname);
	free(directory);
	if (suite)
		free(suite);

	if (valid) {
		return 0;
	} else {
		di_log(DI_LOG_LEVEL_DEBUG, "architecture not supported by selected mirror");
		debconf_input(debconf, "critical", DEBCONF_BASE "noarch");
		if (debconf_go(debconf) == 30)
			exit(10); /* back up to menu */
		else
			return 1; /* back to beginning of questions */
	}
}

int main (int argc, char **argv) {
	int i;
	/* Use a state machine with a function to run in each state */
	int state = 0;
	int (*states[])() = {
		check_base_on_cd,
		choose_protocol,
		get_protocol,
		choose_country,
		set_country,
		choose_mirror,
		validate_mirror,
		choose_proxy,
		set_proxy,
		choose_suite,
		set_codename,
		check_arch,
		NULL,
	};

	if (argc > 1 && strcmp(argv[1], "-n") == 0)
		show_progress = 0;

	debconf = debconfclient_new();
	debconf_capb(debconf, "backup");
	debconf_version(debconf, 2);

	di_system_init("choose-mirror");

	while (state >= 0 && states[state]) {
		int res;

		res = states[state]();
		if (res == 9) /* back up */
			state--;
		else if (res) /* back up to start */
			state = 0;
		else if (debconf_go(debconf)) /* back up */
			state--;
		else
			state++;
	}

	for (i=0; releases[i].name != NULL; i++) {
		free(releases[i].name);
		free(releases[i].suite);
	}

	return (state >= 0) ? 0 : 10; /* backed all the way out */
}
