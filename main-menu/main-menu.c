#include "main-menu.h"

#include <stdlib.h>
#include <search.h>
#include <stdio.h>

/* For btree. */
int package_compare (const void *p1, const void *p2) {
	return strcmp(((struct package_t *)p1)->package,
		      ((struct package_t *)p2)->package);
}

/*
 * qsort comparison function (sort by menu item values, fallback to lexical
 * sort to resolve ties deterministically).
 */
int compare (const void *a, const void *b) {
	int r=(*(struct package_t **)a)->installer_menu_item -
	      (*(struct package_t **)b)->installer_menu_item;
	if (r) return r;
	return strcmp((*(struct package_t **)b)->package,
		      (*(struct package_t **)a)->package);
}

static void order(struct package_t *p, void *package_tree,
	            struct package_t **head, struct package_t **tail) {
	struct package_t dep;
	void *found;
	int i;
	
	if (p->processed)
		return;
	
	for (i=0; p->depends[i] != 0; i++) {
		dep.package = p->depends[i];
		if ((found = tfind(&dep, &package_tree, package_compare)))
			order(*(struct package_t **)found, package_tree, head, tail);
	}
	
	if (*head) {
		(*tail)->next = *tail = p;
		(*tail)->next = NULL;
	}
	else
		*head = *tail = p;
	p->processed = 1;
}

/* Orders the main menu. Returns a linked list of packages. */
struct package_t *main_menu(struct package_t *packages) {
	struct package_t **package_list, *p, *head = NULL, *tail = NULL;
	int i = 0, num = 0;
	void *package_tree = NULL;
	
	/* Make a flat list of the packages, plus a btree for name lookup. */
	for (p = packages; p; p = p->next) {
		tsearch(p, &package_tree, package_compare);
		num++;
	}
	package_list = malloc(num * sizeof(struct package_t *));
	for (p = packages; p; p = p->next) {
		package_list[i] = p;
		i++;
	}
	
	/* Sort by menu number. */
	qsort(package_list, num, sizeof(struct package_t *), compare);
	
	/* Order menu so depended-upon packages come first (topo-sort). */
	/* The menu number is really only used to break ties. */
	for (i = 0; i < num ; i++) {
		if (package_list[i]->installer_menu_item) {
			order(package_list[i], package_tree, &head, &tail);
		}
	}

	return head;
}

int main (int argc, char **argv) {
	struct package_t *first, *p, *packages;
	
	packages = status_read();
	first = main_menu(packages);

	for (p = first; p; p=p->next) {
		if (p->installer_menu_item)
			printf("--%s\n", p->description);
	}
	
	return(0);
}
