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
        struct package_t *menudefault = NULL;
	struct language_description *langdesc;
	int i = 0, num = 0;
	char *s;
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

			if (isdefault(p)) {
                          if (menudefault) {
                            if (menudefault->installer_menu_item > p->installer_menu_item) {
                              menudefault = p;
                            }
                          } else {
                            menudefault = p;
                          }
                        }
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
		debconf->command(debconf, "SET", MAIN_MENU, menudefault->description, NULL);
	debconf->command(debconf, "FSET", MAIN_MENU, "seen", "false", NULL);
	debconf->command(debconf, "SUBST", MAIN_MENU, "MENU", menutext, NULL);
	if (menudefault)
		debconf->command(debconf, "SUBST", MAIN_MENU, "DEFAULT", menudefault->description, NULL);
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

static int config_package(struct package_t *);

static int provides(struct package_t *p, const char *what)
{
	int i;

	for (i = 0; p->provides[i] != 0; i++)
		if (strcmp(p->provides[i], what) == 0)
			return 1;
	return 0;
}

/*
 * Satisfy the dependencies of a virtual package. Its dependencies that actually
 * provide the package are presented in a debconf select question for the user
 * to pick and choose. Other dependencies are just fed recursively through
 * config_package.
 */
static int satisfy_virtual(struct package_t *p) {
	struct debconfclient *debconf;
	struct package_t *dep, *defpkg = NULL;
	char *configcommand;
	int i, ret;
	char *choices, *defval;
	size_t c_size = 1;
	int is_menu_item = 0;

	choices = malloc(1);
	choices[0] = '\0';
	/* Compile a list of providing package. The default choice will be the
	 * package with highest priority. If we have ties, menu items are
	 * preferred. If we still have ties, the default choice is arbitrary */
	for (i = 0; p->depends[i] != 0; i++) {
		if ((dep = tree_find(p->depends[i])) == NULL)
			continue;
		if (!provides(dep, p->package)) {
			/* Non-providing dependency */
			if (dep->status != installed && !config_package(dep))
				return 0;
			continue;
		}
		if (dep->status == installed) {
			/* This means that a providing package is already
			 * configure. So we short-circuit. */
			choices[0] = '\0';
			break;
		}
		if (defpkg == NULL || dep->priority > defpkg->priority ||
				(dep->priority == defpkg->priority &&
				 dep->installer_menu_item))
			defpkg = dep;
		/* This only makes sense if one of the dependencies
		 * is a menu item */
		if (dep->installer_menu_item)
			is_menu_item = 1;
		c_size += strlen(dep->description) + 2;
		choices = realloc(choices, c_size);
		strcat(choices, dep->description);
		strcat(choices, ", ");
	}
	if (c_size >= 3)
		choices[c_size-3] = '\0';
	if (choices[0] != '\0') {
		if (is_menu_item) {
			/* Only let the user choose if one of them is a menu item */
			if (defpkg != NULL)
				defval = defpkg->description;
			else
				defval = "";
			debconf = debconfclient_new();
			debconf->command(debconf, "SUBST", MISSING_PROVIDE,
					"CHOICES", choices, NULL);
			debconf->command(debconf, "SUBST", MISSING_PROVIDE,
					"DEFAULT", defval, NULL);
			debconf->command(debconf, "INPUT medium", MISSING_PROVIDE,
					NULL);
			debconf->command(debconf, "GO", NULL);
			debconf->command(debconf, "GET", MISSING_PROVIDE, NULL);
		}
		/* Go through the dependencies again */
		for (i = 0; p->depends[i] != 0; i++) {
			if ((dep = tree_find(p->depends[i])) == NULL)
				continue;
			if (!provides(dep, p->package))
				continue;
			if (!is_menu_item || strcmp(debconf->value, dep->description) == 0) {
				/* Ick. If we have a menu item it has to match the
				 * debconf choice, otherwise we configure all of
				 * the providing packages */
				if (!config_package(dep))
					return 0;
				if (is_menu_item)
					break;
			}
		}
	}
	free(choices);
	/* And finally configure the virtual package itself */
	asprintf(&configcommand, DPKG_CONFIGURE_COMMAND " %s", p->package);
	ret = SYSTEM(configcommand);
	free(configcommand);
        p->status = (ret == 0) ? installed : half_configured;
	return !ret;
}

/*
 * Simplistic test for virtual packages. A package is virtual if any
 * of its dependencies provides it.
 */
static int is_virtual(struct package_t *p) {
	int i;
	struct package_t *dep;

	for (i = 0; p->depends[i] != 0; i++) {
		if ((dep = tree_find(p->depends[i])) == NULL)
			continue;
		if (provides(dep, p->package))
			return 1;
	}
	return 0;
}

/*
 * Configure all dependencies, special case for virtual packages.
 * This is done depth-first.
 */
static int
config_package(struct package_t *p) {
	char *configcommand;
	int ret, i;
	struct package_t *dep;

	for (i = 0; p->depends[i] != 0; i++) {
		if ((dep = tree_find(p->depends[i])) == NULL)
			continue;
		if (dep->status == installed)
			continue;
		if (is_virtual(dep)) {
			if (!satisfy_virtual(dep))
				return 0;
		} else {
			/* Recursively configure this package */
			if (!config_package(dep))
				return 0;
		}
	}
	asprintf(&configcommand, DPKG_CONFIGURE_COMMAND " %s", p->package);
	ret = SYSTEM(configcommand);
	free(configcommand);
        p->status = (ret == 0) ? installed : half_configured;
	return !ret;
}

int do_menu_item(struct package_t *p) {
	char *configcommand;
	int ret;

	if (p->status == installed) {
		/* The menu item is already configured, so reconfigure it. */
		asprintf(&configcommand, "dpkg-reconfigure %s", p->package);
		ret = SYSTEM(configcommand);
		free(configcommand);
		return !ret;
	}
	else if (p->status == unpacked || p->status == half_configured) {
		config_package(p);
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
