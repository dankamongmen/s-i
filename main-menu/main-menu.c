/*
 * Debian Installer main menu program.
 *
 * Copyright 2000  Joey Hess <joeyh@debian.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "main-menu.h"
#include <cdebconf/debconfclient.h>
#include <stdlib.h>
#include <search.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#ifdef DODEBUG
static int do_system(const char *cmd) {
	DPRINTF("cmd is %s\n", cmd);
	return system(cmd);
}
#endif

/*
 * qsort comparison function (sort by menu item values, fallback to lexical
 * sort to resolve ties deterministically).
 */
int compare (const void *a, const void *b) {
	/* Sometimes, I wish I was writing perl. */
	int r=(*(struct package_t **)a)->installer_menu_item -
	      (*(struct package_t **)b)->installer_menu_item;
	if (r) return r;
	return strcmp((*(struct package_t **)b)->package,
		      (*(struct package_t **)a)->package);
}

/*
 * Builds a linked list of packages, ordered by dependencies, so
 * depended-upon packages come first. Pass the package to start ordering
 * at. head points to the start of the ordered list of packages, and tail
 * points to the end of the list (generally pass in pointers to NULL, unless
 * successive calls to the function are needed to build up a larger list).
 */
void order(struct package_t *p, struct package_t **head, struct package_t **tail) {
	struct package_t *found;
	int i;
	
	if (p->processed)
		return;
	
	for (i=0; p->depends[i] != 0; i++) {
		if ((found = tree_find(p->depends[i])))
			order(found, head, tail);
	}
	
	if (*head)
		(*tail)->next = *tail = p;
	else
		*head = *tail = p;
	
	(*tail)->next = NULL;
		
	p->processed = 1;
}

/*
 * Call this function after calling order to clear the processed tags.
 * Otherwise, later calls to order won't work.
 */
void order_done(struct package_t *head) {
	struct package_t *p;
	
	for (p=head; p; p=p->next)
		p->processed = 0;
}

/* Returns true if the given package could be the default menu item. */
int isdefault(struct package_t *p) {
	char *menutest, *cmd;
	struct stat statbuf;
	int ret;

	asprintf(&menutest, DPKGDIR "info/%s.menutest", p->package);
	if (stat(menutest, &statbuf) == 0) {
		asprintf(&cmd, "%s >/dev/null 2>&1", menutest);
		ret = !SYSTEM(cmd);
		free(cmd);
	}
	else if (p->status == unpacked || p->status == half_configured) {
		ret = 1;
	}
	else {
		ret = 0;
	}
	free(menutest);
	return ret;
}

/* Displays the main menu via debconf and returns the selected menu item. */
struct package_t *show_main_menu(struct package_t *packages) {
	static struct debconfclient *debconf = NULL;
	char *language = NULL;
	struct package_t **package_list, *p, *head = NULL, *tail = NULL;
	struct language_description *langdesc;
	int i = 0, num = 0;
	char *s, *menudefault = NULL;
	char menutext[1024];

	if (! debconf)
		debconf = debconfclient_new();
	
	debconf->command(debconf, "GET", "debian-installer/language", NULL);
	if (language)
		free(language);
        if (debconf->value)
          language = strdup(debconf->value);
	/* Make a flat list of the packages. */
	for (p = packages; p; p = p->next)
		num++;
	package_list = malloc(num * sizeof(struct package_t *));
	for (p = packages; p; p = p->next) {
		package_list[i] = p;
		i++;
	}
	
	/* Sort by menu number. */
	qsort(package_list, num, sizeof(struct package_t *), compare);
	
	/* Order menu so depended-upon packages come first. */
	/* The menu number is really only used to break ties. */
	for (i = 0; i < num ; i++) {
		if (package_list[i]->installer_menu_item) {
			order(package_list[i], &head, &tail);
		}
	}
	order_done(head);

	free(package_list);
	
	/*
	 * Generate list of menu choices for debconf. Also figure out which
	 * is the default.
	 */
	s = menutext;
	for (p = head; p; p = p->next) {
		if (p->installer_menu_item) {
			int ok = 0;
			if (language) {
				langdesc = p->localized_descriptions;
				while (langdesc) {
					if (strcmp(langdesc->language,language) == 0) {
						/* Use this description */
						strcpy(s,langdesc->description);
						s += strlen(langdesc->description);
						ok = 1;
						break;
					}
					langdesc = langdesc->next;
				}
			}
			if (ok == 0) {
				strcpy(s, p->description);
				s += strlen(p->description);
			}
			*s++ = ',';
			*s++ = ' ';

			if (! menudefault && isdefault(p))
				menudefault = p->description;
		}
	}
	/* Trim trailing ", " */
	if (s > menutext)
		s = s - 2;
	*s = 0;
	s = menutext;

	/* Make debconf show the menu and get the user's choice. */
        debconf->command(debconf, "TITLE", "Debian Installer Main Menu", NULL);
	if (menudefault)
		debconf->command(debconf, "SET", MAIN_MENU, menudefault, NULL);
	debconf->command(debconf, "FSET", MAIN_MENU, "seen", "false", NULL);
	debconf->command(debconf, "SUBST", MAIN_MENU, "MENU", menutext, NULL);
	if (menudefault)
		debconf->command(debconf, "SUBST", MAIN_MENU, "DEFAULT", menudefault, NULL);
	debconf->command(debconf, "INPUT medium", MAIN_MENU, NULL);
	debconf->command(debconf, "GO", NULL);
	debconf->command(debconf, "GET", MAIN_MENU, NULL);
	s=debconf->value;
	
	/* Figure out which menu item was selected. */
	for (p = head; p; p = p->next) {
		if (p->installer_menu_item && strcmp(p->description, s) == 0)
			return p;
		else if (p->installer_menu_item && language) {
			for (langdesc = p->localized_descriptions; langdesc; langdesc = langdesc->next)
				if (strcmp(langdesc->language,language) == 0 &&
				    strcmp(langdesc->description,s) == 0) {
					return p;
				}
		}
	}
	return NULL;
}

int do_menu_item(struct package_t *p) {
	char *configcommand;
	struct package_t *head = NULL, *tail = NULL;
	int ret;
	
	if (p->status == installed) {
		/* The menu item is already configured, so reconfigure it. */
		asprintf(&configcommand, "dpkg-reconfigure %s", p->package);
		ret = SYSTEM(configcommand);
		free(configcommand);
		return !ret;
	}
	else if (p->status == unpacked || p->status == half_configured) {
		/*
		 * The menu item is not yet configured. Make sure everything
		 * it depends on is configured, then configure it.
		 */
		order(p, &head, &tail);
		order_done(head);
		for (p = head; p; p = p->next) {
			if (p->status == unpacked || p->status == half_configured) {
				asprintf(&configcommand, DPKG_CONFIGURE_COMMAND " %s", p->package);
				ret = SYSTEM(configcommand);
				free(configcommand);
				if (ret != 0)
					return 0; /* give up on failure */
			}
		}
	}

	return 1;
}

int main (int argc, char **argv) {
	struct package_t *p, *packages;
	
	/* Tell udpkg to shut up. */
	setenv("UDPKG_QUIET", "y", 1);
	
	packages = status_read();
	while ((p=show_main_menu(packages))) {
		do_menu_item(p);
		packages = status_read();
	}
	
	return(0);
}
