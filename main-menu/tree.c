/*
 * Package btree routines.
 */

#include "main-menu.h"

#include <search.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void *root=NULL;

int tree_compare (const void *p1, const void *p2) {
	return strcmp(((struct package_t *)p1)->package,
		      ((struct package_t *)p2)->package);
}

/* Find a package. */
struct package_t *tree_find(char *packagename) {
	struct package_t find;
	void *ret;

	find.package = packagename;
	ret=tfind(&find, &root, tree_compare);
	if (ret)
		return *(struct package_t **)ret;
	return NULL;
}

/* 
 * Adds a package to the tree. Just feed in the package's name; the
 * struct to hold it is automatically created.
 */
struct package_t *tree_add(const char *packagename) {
	struct package_t *pkg=malloc(sizeof(struct package_t));
	memset(pkg, 0, sizeof(struct package_t));
	pkg->package = strdup(packagename);
	return tsearch(pkg, &root, tree_compare);
}

/* Clears out the entire tree, freeing all package stucts contained it in. */
void tree_clear() {
	/* TODO */
}
