/*
 * Retriever interface code.
 */

#include <cdebconf/debconfclient.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "anna.h"

static int
is_retriever(struct package_t *p)
{
	int i;

	for (i = 0; p->provides[i] != 0; i++)
		if (strcmp(p->provides[i], "retriever") == 0)
			return 1;
	return 0;
}

static struct package_t *
get_retriever_packages(void)
{
	FILE *fp;
	struct package_t *p, *q;

	fp = fopen(STATUS_FILE, "r");
	p = di_pkg_parse(fp);
	fclose(fp);
	while (!is_retriever(p))
		p = p->next;
	q = p;
	while (q != NULL && q->next != NULL)
	{
		while (1)
		{
			if (q->next == NULL)
				break;
			// FIXME: Explain why it can't be half-configured
			if (!is_retriever(q->next) || q->next->status == half_configured)
				q->next = q->next->next;
			else
				break;
		}
		q = q->next;
	}
	return p;
}

/* helper function for chosen_retriever() */
/* TODO: i18n */
static char *
get_retriever_choices(struct package_t *p)
{
	int retstr_size = 1;
	char *ret_choices;

	ret_choices = malloc(1);
	ret_choices[0] = '\0';
	for (; p != NULL; p = p->next)
	{
		retstr_size += strlen(p->package) + 2 + strlen(p->description) + 2;
		ret_choices = realloc(ret_choices, retstr_size);
		strcat(ret_choices, p->package);
		strcat(ret_choices, ": ");
		strcat(ret_choices, p->description);
		strcat(ret_choices, ", ");
	}
	if (retstr_size >= 3)
		ret_choices[retstr_size-3] = '\0';
	return ret_choices;
}

static char *
get_chosen_retriever(void)
{
	struct debconfclient *debconf;
	char *retriever = NULL, *colon_p = NULL;

	debconf = debconfclient_new();
	debconf->command(debconf, "GET", ANNA_RETRIEVER, NULL);
	if (debconf->value != NULL)
		colon_p = strchr(debconf->value, ':');
	if (colon_p != NULL)
	{
		retriever = malloc(strlen(RETRIEVER_DIR "/") +
				colon_p - debconf->value + 1);
		strcpy(retriever, RETRIEVER_DIR "/");
		strncat(retriever, debconf->value, colon_p - debconf->value);
	}
	debconfclient_delete(debconf);
	return retriever;
}

static int
choose_retriever(void)
{
	struct debconfclient *debconf;
	struct package_t *ret_pkgs;
	char *ret_choices;

	ret_pkgs = get_retriever_packages();
	if (ret_pkgs == NULL)
		return 0;
	ret_choices = get_retriever_choices(ret_pkgs);
	if (ret_choices[0] == '\0')
		return 0;

	debconf = debconfclient_new();
	debconf->command(debconf, "TITLE", "Choose Retriever", NULL);
	debconf->command(debconf, "FSET", ANNA_RETRIEVER, "seen", "false",
		NULL);
	debconf->command(debconf, "SUBST", ANNA_RETRIEVER, "CHOICES",
			ret_choices, NULL);
	debconf->command(debconf, "INPUT medium", ANNA_RETRIEVER, NULL);
	debconf->command(debconf, "GO", NULL);

	free(ret_choices);

	return 1;
}


/* Ask the chosen retriever to download a particular package to to dest. */
int get_package (struct package_t *package, char *dest) {
	int ret;
	char *retriever;
	char *command;

	retriever = get_chosen_retriever();
	asprintf(&command, "%s retrieve %s %s", retriever, package->filename, dest);
	free(retriever);
	ret=! system(command);
	free(command);
	return ret;
}

/* The location of a downloaded Packages file. Used by try_get_packages
 * and get_packages. Damn, it'd be neat if you could nest function
 * definitions. */
static char tmp_packages[] = DOWNLOAD_DIR "/Packages";

/*
 * Try getting a Packages file, with a given extension. If the extension
 * is not "", unpack_cmd must be the name of a program for unpacking the
 * file. It has to follow the convention of gunzip and bunzip2 and turn
 * 'Packages.xyz' into 'Packages'. If unpack_cmd is NULL, the downloaded
 * file will be left intact.
 */
static int
try_get_packages(char *dist, char *suite, char *ext, char *unpack_cmd)
{
	char *tmp_packages_ext;
	char *retriever;
	char *file, *command;
	int ret;

	retriever = get_chosen_retriever();
	unlink(tmp_packages_ext);
	asprintf(&tmp_packages_ext, "%s%s", tmp_packages, ext);
        /* First try the packages command. If that doesn't work, fall back to
         * retrieving the normal location of a Packages file. */
	asprintf(&command, "%s packages %s %s", retriever, tmp_packages_ext,
			ext[0] != '\0' ? ext : ".");
	ret = system(command);
	free(command);
	if (ret != 0) {
		asprintf(&file, "dists/%s/%s/debian-installer/binary-%s/Packages%s",
				dist, suite, ARCH, ext);
		asprintf(&command, "%s retrieve %s %s", retriever, file, tmp_packages_ext);
		free(file);
		ret = system(command);
		free(command);
	}
	if (unpack_cmd != NULL && ret == 0) {
		/* This means we have Packages.XYZ */
		unlink(tmp_packages);
		asprintf(&command, "%s %s", unpack_cmd, tmp_packages_ext);
		ret = system(command);
		free(command);
		unlink(tmp_packages_ext);
		if (ret != 0)
			unlink(tmp_packages);
	}
	free(tmp_packages_ext);

	return ret;
}

/*
 * Ask the chosen retriever to download the Packages file, and parses it,
 * returning a linked list of package_t structures, or NULL if it fails.
 */
struct package_t *get_packages (void) {
	struct debconfclient *debconf;
	FILE *packages;
	struct package_t *p = NULL, *newp, *plast;
        /* This is a workaround until d-i gets Release files, at which point
           we should parse them instead */
	char *dist;
        char *suites[] =  { "main", "local", 0 };
        char *suite;
        int currsuite = 0;

	choose_retriever();
	debconf = debconfclient_new();
	debconf->command(debconf, "GET", "mirror/distribution", NULL);
	dist = debconf->value;
        suite = suites[currsuite];
        for (; suite != NULL; suite = suites[++currsuite]) {
                packages = NULL;
		if (try_get_packages(dist, suite, ".gz", "gunzip") == 0)
                    packages = fopen(tmp_packages, "r");
		if (packages == NULL && try_get_packages(dist, suite, "", NULL) == 0)
			packages = fopen(tmp_packages, "r");
		if (packages == NULL)
			continue;
                newp = di_pkg_parse(packages);
                fclose(packages);
                unlink(tmp_packages);

                if (newp != NULL) {
                    if (p == NULL) {
                        p = newp;
                        plast = newp;
                    } else
                        plast->next = newp;
                    while (plast->next != NULL)
                        plast = plast->next;
                }
        }
	return p;
}

/* Corresponds to the retriever command 'cleanup' */
void
cleanup(void)
{
	char *retriever;
	char *command;

	retriever = get_chosen_retriever();
	asprintf(&command, "%s cleanup", retriever);
	free(retriever);
	system(command);
	free(command);
}
