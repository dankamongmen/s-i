#include "main-menu.h"

#include <stdlib.h>
#include <search.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

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

/* Returns true if the given package could be the default menu item. */
int isdefault(struct package_t *p) {
	char menutest[1024];
	struct stat statbuf;

	sprintf(menutest, DPKGDIR "info/%s.menutest", p->package);
	if (stat(menutest, &statbuf) == 0) {
		return ! system(menutest);
	}
	else if (p->status == STATUS_UNPACKED) {
		return 1;
	}
	return 0;
}

/* Displays the main menu via debconf. */
struct package_t *show_main_menu(struct package_t *packages) {
	struct package_t **package_list, *p, *head = NULL, *tail = NULL;
	int i = 0, num = 0;
	void *package_tree = NULL;
	char *s, *menudefault = NULL;
	char menutext[1024];
	
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
	
	/*
	 * Generate list of menu choices for debconf. Also figure out which
	 * is the default.
	 */
	s = menutext;
	for (p = head; p; p = p->next) {
		if (p->installer_menu_item) {
			strcpy(s, p->description);
			s += strlen(p->description);
			*s++ = ',';
			*s++ = ' ';

			if (! menudefault && isdefault(p))
				menudefault = p->description;
		}
	}
	/* Trim trailing ", " */
	s = s - 2;
	*s = 0;
	s = menutext;
	
	/* Make debconf show the menu and get the user's choice. */
	if (menudefault)
		debconf_command("SET %s %s", MAIN_MENU, menudefault);
	debconf_command("FSET %s isdefault true", MAIN_MENU);
	debconf_command("SUBST %s MENU %s", MAIN_MENU, menutext);
	debconf_command("INPUT medium %s", MAIN_MENU);
	debconf_command("GO");
	debconf_command("GET %s", MAIN_MENU);
	s=debconf_ret();

	/* Figure out which menu item was selected. */
	for (p = head; p; p = p->next) {
		if (p->installer_menu_item && strcmp(p->description, s) == 0)
			return p;
	}
	return 0; // should never happen
}

int main (int argc, char **argv) {
	struct package_t *p, *packages;
	
	packages = status_read();
	p=show_main_menu(packages);

	fprintf(stderr, "%s\n", p->package);
	
	return(0);
}
