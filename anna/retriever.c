/*
 * Retriever interface code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "anna.h"

/* Returns the filename of the retriever to use. */
/* TODO: handle more than one, and don't hard-code. */
char *chosen_retriever (void) {
	return "usr/lib/debian-installer/retriever/wget-retriever";
}

/* Ask the chosen retriever to download a particular package to to dest. */
int get_package (struct package_t *package, char *dest) {
	int ret;
	char *retriever=chosen_retriever();
	char *command=malloc(strlen(retriever) + 1 + strlen(package->filename) +
				     strlen(dest) + 1);
	sprintf(command, "%s %s %s", retriever, package->filename, dest);
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
	char *command=malloc(strlen(retriever) + 10 + sizeof(tmp_packages) + 1);

	unlink(tmp_packages);
	sprintf(command, "%s Packages %s", retriever, tmp_packages);
	if (system(command) != 0) {
		free(command);
		return NULL;
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
	
	return p;
}

