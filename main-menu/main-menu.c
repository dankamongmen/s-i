#include "main-menu.h"

#include <stdlib.h>
#include <search.h>
#include <stdio.h>

/*
 * qsort comparison function (sort by menu item values, fallback to lexical
 * sort to resolve ties deterministically).
 */
int compare (const void *a, const void *b) {
	int r=((struct package_t *)a)->installer_menu_item -
	      ((struct package_t *)b)->installer_menu_item;
	if (r) return r;
	return strcmp(((struct package_t *)a)->package,
		      ((struct package_t *)b)->package);
}

/* For btree. */
int package_compare (const void *p1, const void *p2) {
	return strcmp(((struct package_t *)p1)->package,
		      ((struct package_t *)p2)->package);
}

static void order(struct package_t *p, void *packages,
	            struct package_t **head, struct package_t **tail) {
	struct package_t dep, *d;
	int i;
	
	if (p->processed)
		return;
	
	for (i=0; p->depends[i] != 0; i++) {
		dep.package = p->depends[i];
		if ((d=tfind(&dep, &packages, package_compare)))
			order(d, packages, head, tail);
	}
	
	if (*head)
		(*tail)->next = *tail = p;
	else
		*head = *tail = p;
	p->processed = 1;
}

/* Orders the main menu. Returns a linked list of packages. */
struct package_t *main_menu(struct package_t *package_list) {
	struct package_t *p, *head = NULL, *tail = NULL;
	void *ptree = 0;
	
	/* Find number of packages and also generate btree of them. */
	for (p = package_list; p->package != 0; p++) {
		tsearch(p, &ptree, package_compare);
	}
		
	/* Sort by menu number. */
	qsort(package_list, p - package_list, sizeof(struct package_t), compare);

	/* Order menu so depended-upon packages come first (topo-sort). */
	/* The menu number is really only used to break ties. */
	for (p = package_list; p->package != 0; p++) {
		if (p->installer_menu_item)
			order(p, ptree, &head, &tail);
	}

	return head;
}

int main (int argc, char **argv) {
	struct package_t *packages, *first, *p;
	
	packages = status_read();	
	first = main_menu(packages);
	
	for (p = first; p; p=p->next) {
		printf("--%s\n", p->package);
	}
	
	return(0);
}
