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

#define STATUS_FILE "/var/lib/dpkg/status"

static struct package_t *
get_retriever_packages(void)
{
	FILE *fp;
	struct package_t *p, *q;

	fp = fopen(STATUS_FILE, "r");
	p = di_pkg_parse(fp);
	fclose(fp);
	while (p->provides == NULL || strstr(p->provides, "retriever") == NULL)
		p = p->next;
	q = p;
	while (q != NULL && q->next != NULL)
	{
		while (1)
		{
			if (q->next == NULL)
				break;
			if (q->next->provides == NULL
					||  strstr(q->next->provides, "retriever") == NULL
					||  (q->next->status != unpacked &&
						q->next->status != installed)
			   )
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
				colon_p - debconf->value);
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
	char *ret_choices, *ret_default;

	ret_pkgs = get_retriever_packages();
	if (ret_pkgs == NULL)
		return 0;
	ret_choices = get_retriever_choices(ret_pkgs);
	if (ret_choices[0] == '\0')
		return 0;
	debconf = debconfclient_new();
	debconf->command(debconf, "GET", ANNA_RETRIEVER, NULL);
	if (debconf->value != NULL)
		ret_default = debconf->value;
	else
	{
		ret_default = strrchr(ret_choices, ',');
		if (ret_default != NULL)
			ret_default += 2;
	}

	debconf->command(debconf, "TITLE", "Choose Retriever", NULL);
	debconf->command(debconf, "SET", ANNA_RETRIEVER, ret_default,
			NULL);
	debconf->command(debconf, "SUBST", ANNA_RETRIEVER, "CHOICES",
			ret_choices, NULL);
	debconf->command(debconf, "SUBST", ANNA_RETRIEVER, "DEFAULT",
			ret_default, NULL);
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
	asprintf(&command, "%s %s %s", retriever, package->filename, dest);
	free(retriever);
	ret=! system(command);
	free(command);
	return ret;
}

/*
 * Ask the chosen retriever to download the Packages file, and parses it,
 * returning a linked list of package_t structures, or NULL if it fails.
 *
 * TODO: compressed Packages files?
 */
struct package_t *get_packages (void) {
	char *retriever;
	FILE *packages;
	struct package_t *p = NULL, *newp, *plast;
	static char tmp_packages[] = DOWNLOAD_DIR "/Packages";
        /* This is a workaround until d-i gets Release files, at which point
           we should parse them instead */
        char *suites[] =  { "main", "local", 0 };
        char *suite;
        int currsuite = 0;

	choose_retriever();
	retriever = get_chosen_retriever();
        suite = suites[currsuite];
        while (suite != NULL) {
		char *command;

                unlink(tmp_packages);
                asprintf(&command, "%s Packages %s %s", retriever, tmp_packages, 
                        suite);
                fprintf(stderr,"%s\n", command);
                if (system(command) != 0) {
                        free(command);
                        unlink(tmp_packages);
                        suite = suites[++currsuite];
/*                        return NULL;*/
                        continue;
                }
                packages=fopen(tmp_packages, "r");
                free(command);
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

                suite = suites[++currsuite];
        }
	free(retriever);
	return p;
}

