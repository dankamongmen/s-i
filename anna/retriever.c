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

/* helper function for chosen_retriever() */
static char *
get_retriever_names()
{
	DIR *retrdir;
	struct dirent *d_ent;
	int retstr_size = 1, fname_len;
	char *ret_choices;
	char *fname;
	struct stat st;

	/* Find out which retrievers are available */
	retrdir = opendir(RETRIEVER_DIR);
	ret_choices = malloc(1);
	ret_choices[0] = '\0';
	while ((d_ent = readdir(retrdir)) != NULL) {
		fname_len = strlen(d_ent->d_name);
		fname = (char *)malloc(strlen(RETRIEVER_DIR) + 1 + 
				fname_len);
		strcpy(fname, RETRIEVER_DIR "/");
		strcat(fname, d_ent->d_name);
		stat(fname, &st);
		free(fname);
		if (S_ISREG(st.st_mode)) {
			/* Should we check for x flag too? */
			retstr_size += fname_len + 2;
			ret_choices = realloc(ret_choices, retstr_size);
			strcat(ret_choices, d_ent->d_name);
			strcat(ret_choices, ", ");
		}
	}
	closedir(retrdir);
	if (retstr_size >= 3)
		ret_choices[retstr_size-3] = '\0';
	return ret_choices;
}

/* Returns the filename of the retriever to use. */
/* TODO: error handling */
char *chosen_retriever (void) {
	static char *retriever = NULL;

	if (retriever == NULL) {
		struct debconfclient *debconf;
		char *ret_choices, *ret_default;

		ret_choices = get_retriever_names();
		ret_default = strrchr(ret_choices, ',');
		if (ret_default != NULL)
			ret_default += 2;

		debconf = debconfclient_new();
		debconf->command(debconf, "TITLE", "Choose Retriever", NULL);
		debconf->command(debconf, "SET", ANNA_RETRIEVER, ret_default, 
				NULL);
		debconf->command(debconf, "SUBST", ANNA_RETRIEVER, "RETRIEVER", 
				ret_choices, NULL);
		debconf->command(debconf, "SUBST", ANNA_RETRIEVER, "DEFAULT", 
				ret_default, NULL);
		debconf->command(debconf, "INPUT medium", ANNA_RETRIEVER, NULL);
		debconf->command(debconf, "GO", NULL);
		debconf->command(debconf, "GET", ANNA_RETRIEVER, NULL);
		retriever = malloc(strlen(RETRIEVER_DIR) + 1 + 
                                   strlen(debconf->value));
		strcpy(retriever, RETRIEVER_DIR "/");
		strcat(retriever, debconf->value);
		debconfclient_delete(debconf);
                
		free(ret_choices);
        }
        return retriever;
}

/* Ask the chosen retriever to download a particular package to to dest. */
int get_package (struct package_t *package, char *dest) {
	int ret;
	char *retriever=chosen_retriever();
	char *command;

	asprintf(&command, "%s %s %s", retriever, package->filename, dest);
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
	char *retriever=chosen_retriever();
	FILE *packages;
	struct package_t *p = NULL, *newp, *plast;
	static char tmp_packages[] = DOWNLOAD_DIR "/Packages";
        /* This is a workaround until d-i gets Release files, at which point
           we should parse them instead */
        char *suites[] =  { "main", "local", 0 };
        char *suite;
        int currsuite = 0;
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
	return p;
}

