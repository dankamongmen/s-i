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

/* Returns the filename of the retriever to use. */
/* TODO: error handling */
char *chosen_retriever (void) {
	static char *retriever = NULL;
        
	if (retriever == NULL) {
                DIR *retrdir;
		struct dirent *d_ent;
		struct debconfclient *debconf;
		int retstr_size = 1, fname_len;
		char *ret_choices, *ret_default = NULL;
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
				/* Pick the first one to be default :) */
				if (ret_default == NULL)
					ret_default = strdup(d_ent->d_name);
			}
                }
		closedir(retrdir);
		if (retstr_size >= 3)
			ret_choices[retstr_size-3] = '\0';

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
		free(ret_default);
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
	char buf[BUFSIZE];
	struct package_t *p = NULL, *newp;
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
                
                while (fgets(buf, BUFSIZE, packages) && !feof(packages)) {
                        buf[strlen(buf)-1] = 0;
                        if (strstr(buf, "Package: ") == buf) {
                                newp=(struct package_t *) malloc(sizeof(struct package_t));
                                newp->next = p;
                                p = newp;
                                p->package = strdup(buf + 9);
                        }
                        else if (strstr(buf, "Installer-Menu-Item: ") == buf) {
                                p->installer_menu_item = atoi(buf + 21);
                        }
                        else if (strstr(buf, "Filename: ") == buf) {
                                p->filename = strdup(buf + 10);
                        }
                        else if (strstr(buf, "MD5sum: ") == buf) {
                                p->md5sum = strdup(buf + 8);
                        }
                }
                
                unlink(tmp_packages);
                suite = suites[++currsuite];
        }
	return p;
}

