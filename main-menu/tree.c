/*
 * Package btree routines.
 */

#include "main-menu.h"

#include <search.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void *root=NULL;

int _tree_compare (const void *p1, const void *p2) {
	return strcmp(((struct package_t *)p1)->package,
		      ((struct package_t *)p2)->package);
}

/* Find a package. */
struct package_t *tree_find(char *packagename) {
	struct package_t find;
	void *ret;

	find.package = packagename;
	ret=tfind(&find, &root, _tree_compare);
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
	/* 
	 * I should just be able to return tsearch's return code, but this
	 * makes my code fail horribly later on. I think tsearch has
	 * issues.
	 */
	tsearch(pkg, &root, _tree_compare);
	return pkg;
}

void _tree_delete(const void *nodep, const VISIT which, const int depth) {
	struct package_t *p;

	switch(which) {
		case postorder:
//			p = *(struct package_t **)nodep;
//			printf("[%i] postorder %p %s\n", depth, nodep, p->package);
			break;
		case preorder:
//			p = *(struct package_t **)nodep;
//			printf("[%i] preorder %p %s\n", depth, nodep, p->package);
			break;
		case endorder:
//			p = *(struct package_t **)nodep;
//			printf("[%i] endorder %p %s\n", depth, nodep, p->package);
//			break;
		case leaf:
			p = *(struct package_t **)nodep;
			printf("[%i] delete %p %s\n", depth, nodep, p->package);
			tdelete(&p, &root, _tree_compare);
			if (p->package)
				free(p->package);
			if (p->description)
				free(p->description);
			free(p);
			break;
	}
}

/* Clears out the entire tree, freeing all package stucts contained it in. */
void tree_clear() {
#if DODEBUG
	tree_dump();
#endif
	twalk(root, _tree_delete);
#if DODEBUG
	tree_dump();
#endif
}

#if DODEBUG
void _tree_dump(const void *nodep, const VISIT which, const int depth) {
	struct package_t *p = *(struct package_t **)nodep;

	switch(which) {
		case postorder:
			DPRINTF("[%i] postorder %p %s\n", depth, nodep, p->package);
			break;
		case preorder:
			DPRINTF("[%i] preorder %p %s\n", depth, nodep, p->package);
			break;
		case endorder:
			DPRINTF("[%i] endorder %p %s\n", depth, nodep, p->package);
			break;
		case leaf:
			DPRINTF("[%i] leaf %p %s\n", depth, nodep, p->package);
			break;
	}
}

/* Dump out the tree. */
void tree_dump() {
	twalk(root, _tree_dump);
}
#endif
