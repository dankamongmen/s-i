#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <search.h>

#include "main-menu.h"

static char **depends_split(const char *dependsstr) {
	static char *dependsvec[DEPENDSMAX];
	char *p;
	int i = 0;

	dependsvec[0] = 0;

	if (dependsstr != 0) {
		p = strdup(dependsstr);
		while (*p != 0 && *p != '\n') {
			if (*p != ' ') {
				if (*p == ',') {
					*p = 0;
					dependsvec[++i] = 0;
				}
				else if (dependsvec[i] == 0)
					dependsvec[i] = p;
			}
			else
				*p = 0; /* eat the space... */
			p++;
		}
		*p = 0;
	}
	dependsvec[i+1] = 0;
	return dependsvec;
}

int package_compare(const void *p1, const void *p2) {
	return strcmp(((struct package_t *)p1)->package,
			((struct package_t *)p2)->package);
}

/* Adds a new package to the tree, unless it already exists. */
struct package_t *_newpackage (const char *packagename, void *status) {
	struct package_t *p, *t = 0;

	p = (struct package_t *)malloc(sizeof(struct package_t));
	memset(p, 0, sizeof(struct package_t));
	p->package = strdup(packagename);
	t = *(struct package_t **)tsearch(p, &status, package_compare);
	if (t->refcount++ > 0) {
		free(p->package);
		free(p);
printf("dup! %s\n", t->package);
	}
	return t;
}

/* 
 * Read status file into memory as a binary tree, preserving only those
 * fields that are needed.
 */
void *status_read(void) {
	FILE *f;
	char buf[BUFSIZE];
	void *status = 0;
	struct package_t *p = 0, *t = 0;
	char **dependsvec;
	int i;

	if ((f = fopen(STATUSFILE, "r")) == NULL) {
		perror(STATUSFILE);
		return 0;
	}

	while (fgets(buf, BUFSIZE, f) && !feof(f)) {
		buf[strlen(buf)-1] = 0;
		if (*buf == 0) {
			p = 0;
		}
		else if (strstr(buf, "Package: ") == buf) {
			p = _newpackage(buf+9, status);
printf("%s %i\n", p->package, p->refcount);
		}
		else if (strstr(buf, "Installer-Menu-Item: ") == buf) {
			p->installer_menu_item=atoi(buf+21);
		}
		else if (strstr(buf, "Description: ") == buf) {
			/* Short description only. */
			p->description = strdup(buf+13);
		}
		else if (strstr(buf, "Depends: ") == buf) {
			/*
			 * A "Depends" triggers the insertation of
			 * stub packages into the status binary-tree (if
			 * the depended-upon package doesn't already exist).
			 * The stubs will be filled out in due course.
			 *
			 * This assumes that the status database is
			 * consistent.
			 */
			dependsvec = depends_split(buf+9);
			i=0;
			while (dependsvec[i] != 0) {
				t = _newpackage(dependsvec[i], status);
				t->requiredfor[t->requiredcount++] = p;
printf("%s required for %s\n", t->package, p->package);
				i++;
			}
		}
		else if (strstr(buf, "Provides: ") == buf) {
			/* Um, how do we handle provides? TODO */
		}
	}
	fclose(f);
	return status;
}
