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
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

static struct linkedlist_t *packages;

#ifdef DODEBUG
static int do_system(const char *cmd) {
	DPRINTF("cmd is %s\n", cmd);
	return system(cmd);
}
#endif

static void update_language (void);

/*
 * qsort comparison function (sort by menu item values, fall back to
 * lexical sort to resolve ties deterministically).
 */
int compare (const void *a, const void *b) {
	/* Sometimes, I wish I was writing perl. */
	int r=(*(struct package_t **)a)->installer_menu_item -
	      (*(struct package_t **)b)->installer_menu_item;
	if (r) return r;
	return strcmp((*(struct package_t **)b)->package,
		      (*(struct package_t **)a)->package);
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

/* Expects a topologically ordered linked list of packages. */
static struct package_t *
get_default_menu_item(struct linkedlist_t *list)
{
	struct package_t *p, *q;
	struct list_node *node;
	int i, cont;

	/* Traverse the list, return the first menu item that isn't installed */
	for (node = list->head; node != NULL; node = node->next) {
		p = (struct package_t *)node->data;
		if (!p->installer_menu_item || p->status == installed)
			continue;
		cont = 0;
                /* Check if a "parallel" package is installed
		 * (netcfg-{static,dhcp} and {lilo,grub}-installer are
		 * examples of parallel packages */
		for (i = 0; p->provides[i] != NULL; i++) {
			q = p->provides[i]->ptr;
			if (q != NULL && di_pkg_is_installed(q)) {
				cont = 1;
				break;
			}
		}
		if (!cont)
			return p;
	}
	/* Severely broken, there are no menu items in the sorted list */
	return NULL;
}

static char *menu_priority = "medium";

/* Displays the main menu via debconf and returns the selected menu item. */
struct package_t *show_main_menu(struct linkedlist_t *list) {
	static struct debconfclient *debconf = NULL;
	char *language = NULL;
	struct package_t **package_list, *p;
	struct linkedlist_t *olist;
	struct list_node *node;
        struct package_t *menudefault = NULL, *ret = NULL;
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
	for (node = list->head; node; node = node->next) {
		p = (struct package_t *)node->data;
		if (p->installer_menu_item)
			num++;
	}
	package_list = malloc(num * sizeof(struct package_t *));
	for (node = list->head; node; node = node->next) {
		p = (struct package_t *)node->data;
		if (p->installer_menu_item)
			package_list[i++] = (struct package_t *)node->data;
	}
	
	/* Sort by menu number. */
	qsort(package_list, num, sizeof(struct package_t *), compare);
	
	/* Order menu so depended-upon packages come first. */
	/* The menu number is really only used to break ties. */
	olist = di_pkg_toposort_arr(package_list, num);

	/*
	 * Generate list of menu choices for debconf.
	 */
	s = menutext;
	for (node = olist->head; node != NULL; node = node->next) {
		int ok = 0;
		p = (struct package_t *)node->data;
		if (!p->installer_menu_item)
			continue;
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
	}
	/* Trim trailing ", " */
	if (s > menutext)
		s = s - 2;
	*s = 0;
	s = menutext;
	menudefault = get_default_menu_item(olist);
	free(olist);

	/* Make debconf show the menu and get the user's choice. */
        debconf->command(debconf, "TITLE", "Debian Installer Main Menu", NULL);
	debconf->command(debconf, "FSET", MAIN_MENU, "seen", "false", NULL);
	debconf->command(debconf, "SUBST", MAIN_MENU, "MENU", menutext, NULL);
	if (menudefault)
		debconf->command(debconf, "SET", MAIN_MENU, menudefault->description, NULL);
	debconf->command(debconf, "INPUT", menu_priority, MAIN_MENU, NULL);
	debconf->command(debconf, "GO", NULL);
	debconf->command(debconf, "GET", MAIN_MENU, NULL);
	s=debconf->value;
	
	/* Figure out which menu item was selected. */
	for (i = 0; i < num; i++) {
		p = package_list[i];
		if (strcmp(p->description, s) == 0) {
			ret = p;
			break;
		} else if (language) {
			for (langdesc = p->localized_descriptions; langdesc; langdesc = langdesc->next)
				if (strcmp(langdesc->language,language) == 0 &&
				    strcmp(langdesc->description,s) == 0) {
					ret = p;
					break;
				}
		}
	}
	free(language);
	free(package_list);
	return ret;
}

static int config_package(struct package_t *);

/*
 * Satisfy the dependencies of a virtual package. Its dependencies
 * that actually provide the package are presented in a debconf select
 * question for the user to pick and choose. Other dependencies are
 * just fed recursively through config_package.
 */
static int satisfy_virtual(struct package_t *p) {
	struct debconfclient *debconf = NULL;
	struct package_t *dep, *defpkg = NULL;
	int i;
	char *choices, *tmp;
	size_t c_size = 1;
	int is_menu_item = 0;

        asprintf(&choices, "");
	/* Compile a list of providing package. The default choice will be the
	 * package with highest priority. If we have ties, menu items are
	 * preferred. If we still have ties, the default choice is arbitrary */
	for (i = 0; p->depends[i] != 0; i++) {
		if ((dep = p->depends[i]->ptr) == NULL)
			continue;
		if (!di_pkg_provides(dep, p)) {
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
		if (dep == defpkg)
			/* We want the default to be the first item */
			asprintf(&tmp, "%s, %s", dep->description, choices);
		else
			asprintf(&tmp, "%s%s, ", choices, dep->description);
		free(choices);
		choices = tmp;
	}
	c_size = strlen(choices);
	if (c_size >= 2)
		choices[c_size-2] = '\0';
	if (choices[0] != '\0') {
		if (is_menu_item) {
			char *priority = "medium";
			/* Only let the user choose if one of them is a menu item */
			debconf = debconfclient_new();
			debconf->command(debconf, "FSET", MISSING_PROVIDE, "seen",
					"false", NULL);
			if (defpkg != NULL)
				debconf->command(debconf, "SET", MISSING_PROVIDE,
						defpkg->description, NULL);
			else
				/* TODO: How to figure out a default? */
				priority = "critical";
			debconf->command(debconf, "SUBST", MISSING_PROVIDE,
					"CHOICES", choices, NULL);
			debconf->command(debconf, "INPUT", priority, MISSING_PROVIDE,
					NULL);
			debconf->command(debconf, "GO", NULL);
			debconf->command(debconf, "GET", MISSING_PROVIDE, NULL);
		}
		/* Go through the dependencies again */
		for (i = 0; p->depends[i] != 0; i++) {
			if ((dep = p->depends[i]->ptr) == NULL)
				continue;
			if (!di_pkg_provides(dep, p))
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
	if (debconf)
		debconfclient_delete(debconf);
	free(choices);
	/* It doesn't make sense to configure virtual packages,
	 * since they are, well, virtual. */
	p->status = installed;
	return 1;
}

static void
check_special(struct package_t *p)
{
	int i;

	/*
	 * A language selection package must provide the virtual
	 * package 'language-selected'.
	 * The LANGUAGE environment variable must be updated
	 */
	for (i = 0; p->provides[i] != NULL; i++)
		if (strcmp(p->provides[i]->name, "language-selected") == 0) {
			update_language();
			break;
		}
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

	if (di_pkg_is_virtual(p)) {
		return satisfy_virtual(p);
	}

	for (i = 0; p->depends[i] != 0; i++) {
		if ((dep = p->depends[i]->ptr) == NULL)
			continue;
		if (dep->status == installed)
			continue;
		/* Recursively configure this package */
		if (!config_package(dep))
			return 0;
	}

	asprintf(&configcommand, DPKG_CONFIGURE_COMMAND " %s", p->package);
	ret = SYSTEM(configcommand);
	free(configcommand);
        if (ret == 0) {
            p->status = installed;
	    check_special(p);
        } else
            p->status = half_configured;
	return !ret;
}

int do_menu_item(struct package_t *p) {
	char *configcommand;
	int ret = 0;

	if (p->status == installed) {
		/* The menu item is already configured, so reconfigure it. */
		asprintf(&configcommand, DPKG_CONFIGURE_COMMAND " --force-configure %s", p->package);
		ret = !SYSTEM(configcommand);
		free(configcommand);
		check_special(p);
	}
	else if (p->status == unpacked || p->status == half_configured) {
		ret = config_package(p);
	}

	return ret;
}

static void update_language (void) {
	struct debconfclient *debconf;

	debconf = debconfclient_new();
	debconf->command(debconf, "GET", "debian-installer/language", NULL);
	if (*debconf->value != 0)
	{
		setenv("LANGUAGE", debconf->value, 1);
		/*  debconf backend must also be updated  */
		kill(getppid(), SIGUSR1);
	}
	debconfclient_delete(debconf);
}

int main (int argc, char **argv) {
	struct package_t *p;

	/* Tell udpkg to shut up. */
	setenv("UDPKG_QUIET", "y", 1);

	packages = status_read();
	while ((p=show_main_menu(packages))) {
		if (!do_menu_item(p)) {
			di_log("Setting main menu question priority to critical");
			menu_priority = "critical";
		}
		di_list_free(packages, di_pkg_free);
		packages = status_read();
	}
	
	return(0);
}
