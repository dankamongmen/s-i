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
#include "stderr-log.h"
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

const int RAISE = 1;
const int LOWER = 0;

/* Save default priority, to be able to return to it when we have to lower it */
int default_priority = 1;

/* Save priority set by main-menu to detect priority changes from the user */
int local_priority = -1;

static struct linkedlist_t *packages;

static struct debconfclient *debconf;

/*
 * qsort comparison function (sort by menu item values, fall back to
 * lexical sort to resolve ties deterministically).
 */
int package_array_compare (const void *v1, const void *v2) {
        di_system_package *p1, *p2;

        p1 = *(di_system_package **)v1;
        p2 = *(di_system_package **)v2;

	int r = p1->installer_menu_item - p2->installer_menu_item;
	if (r) return r;
	return strcmp(p1->p.package, p2->p.package);
}

int isdefault(di_package *p) {
	int check;

        check = di_system_dpkg_package_control_file_exec(p, "menutest", 0, NULL);
	if (!check || p->status == di_package_status_unpacked || p->status == di_package_status_half_configured) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Expects a topologically ordered linked list of packages. */
static di_package *
get_default_menu_item(di_slist *list)
{
	di_package *p;
	di_slist_node *node, *node1;
	int cont;

	/* Traverse the list, return the first menu item that isn't installed */
	for (node = list->head; node != NULL; node = node->next) {
		p = node->data;
		if (!((di_system_package *)p)->installer_menu_item || p->status == di_package_status_installed || !di_system_dpkg_package_control_file_exec(p, "isinstallable", 0, NULL))
			continue;
		/* If menutest says this item should be default, make it so */
		if (!isdefault(p))
			continue;
		cont = 0;
                /* Check if a "parallel" package is installed
		 * (netcfg-{static,dhcp} and {lilo,grub}-installer are
		 * examples of parallel packages */
                for (node1 = p->depends.head; node1; node1 = node1->next) {
                        di_package_dependency *d = node->data;
                        if (d->type == di_package_dependency_type_provides && d->ptr->status == di_package_status_installed) {
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

/* Return the text of the menu entry for PACKAGE, translated to
   LANGUAGE if possible.  */
static size_t menu_entry(struct debconfclient *debconf, char *language, di_package *package, char *buf, size_t size)
{
	char question[256];

	snprintf(question, sizeof(question), "debian-installer/%s/title", package->package);
	if (language) {
		char field[128];

		snprintf(field, sizeof (128), "Description-%s.UTF-8", language);
		if (!debconf_metaget(debconf, question, field))
                {
			strncpy(buf, debconf->value, size);
                        return strlen (buf);
                }
	}
	if (!debconf_metaget(debconf, question, "Description"))
        {
                strncpy(buf, debconf->value, size);
                return strlen (buf);
        }

	/* The following fallback case can go away once all packages
	   have transitioned to the new form.  */
        di_log(DI_LOG_LEVEL_INFO, "Falling back to the package description for %s", package->package);
        strncpy(buf, package->description, size);
        return strlen (buf);
}

/* Displays the main menu via debconf and returns the selected menu item. */
di_package *show_main_menu(di_packages *packages, di_packages_allocator *allocator) {
	char *language = NULL;
	di_package **package_array, *p;
	di_slist *list;
	di_slist_node *node;
        di_package *menudefault = NULL, *ret = NULL;
	int i = 0, num = 0;
	char buf[256], *menu, *s;
        int menu_size, menu_used, size;

	debconf->command(debconf, "GET", "debian-installer/language", NULL);
	if (language)
		free(language);
        if (debconf->value)
          language = strdup(debconf->value);

	for (node = packages->list.head; node; node = node->next) {
		p = node->data;
		if (((di_system_package *)p)->installer_menu_item)
			num++;
	}
	package_array = di_new (di_package *, num + 1);
        package_array[num] = NULL;
	for (node = packages->list.head; node; node = node->next) {
		p = node->data;
		if (((di_system_package *)p)->installer_menu_item)
			package_array[i++] = node->data;
	}
	
	/* Sort by menu number. */
	qsort(package_array, num, sizeof (di_package *), package_array_compare);

	/* Order menu so depended-upon packages come first. */
	/* The menu number is really only used to break ties. */
	list = di_packages_resolve_dependencies_array (packages, package_array, allocator);

	/*
	 * Generate list of menu choices for debconf.
	 */
        menu = di_malloc(1024);
        menu[0] = '\0';
        menu_size = 1024;
        menu_used = 1;
	for (node = list->head; node != NULL; node = node->next) {
		p = node->data;
		if (!((di_system_package *)p)->installer_menu_item || !check_script(p, "isinstallable"))
			continue;
		size = menu_entry(debconf, language, p, buf, sizeof (buf));
                if (menu_used + size + 2 > menu_size)
                {
                  menu_size += 1024;
                  menu = di_realloc(ret, menu_size);
                }
                strcat(menu, buf);
                menu_used += size + 2;
                if (node->next)
                  strcat(menu, ", ");
	}
	menudefault = get_default_menu_item(list);
	di_slist_free(list);

	/* Make debconf show the menu and get the user's choice. */
        debconf->command(debconf, "SETTITLE", "debian-installer/main-menu-title", NULL);
	debconf->command(debconf, "CAPB", NULL);
	debconf->command(debconf, "FSET", MAIN_MENU, "seen", "false", NULL);
	debconf->command(debconf, "SUBST", MAIN_MENU, "MENU", menu, NULL);
	if (menudefault) {
                menu_entry(debconf, language, menudefault, buf, sizeof (buf));
		debconf->command(debconf, "SET", MAIN_MENU, buf, NULL);
	}
	debconf->command(debconf, "INPUT", "medium", MAIN_MENU, NULL);
	debconf->command(debconf, "GO", NULL);
	debconf->command(debconf, "GET", MAIN_MENU, NULL);
	s = strdup(debconf->value);
	
	/* Figure out which menu item was selected. */
	for (i = 0; i < num; i++) {
		p = package_array[i];
		menu_entry(debconf, language, p, buf, sizeof (buf));
		if (strcmp(buf, s) == 0) {
			ret = p;
			break;
		}
	}
	free(language);
	free(package_array);
	return ret;
}

static int check_special(di_package *p);

/*
 * Satisfy the dependencies of a virtual package. Its dependencies
 * that actually provide the package are presented in a debconf select
 * question for the user to pick and choose. Other dependencies are
 * just fed recursively through di_config_package.
 */
static int satisfy_virtual(di_package *p) {
	struct debconfclient *debconf;
	struct package_t *dep, *defpkg = NULL;
	int i;
	char *choice = NULL, *choices, *tmp, *language = NULL;
	size_t c_size = 1;
	int is_menu_item = 0;

	debconf = debconfclient_new();
	debconf->command(debconf, "GET", "debian-installer/language", NULL);
	if (debconf->value)
		language = strdup(debconf->value);

	/* Compile a list of providing package. The default choice will be the
	 * package with highest priority. If we have ties, menu items are
	 * preferred. If we still have ties, the default choice is arbitrary */
	for (i = 0; p->depends[i] != 0; i++) {
		if ((dep = p->depends[i]->ptr) == NULL)
			continue;
		if (!di_pkg_provides(dep, p)) {
			/* Non-providing dependency */
			if (dep->status != installed && !di_config_package(dep, satisfy_virtual, check_special))
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
		if (dep == defpkg) {
			/* We want the default to be the first item */
			char *entry;
			entry = menu_entry(debconf, language, dep);
			if (asprintf(&tmp, "%s, %s", entry, choices) == -1) {
				return 0;
			}
			free(entry);
		} else {
			char *entry;
			entry = menu_entry(debconf, language, dep);
			if (asprintf(&tmp, "%s%s, ", choices, entry) == -1) {
				return 0;
			}
			free(entry);
		}
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
			if (defpkg != NULL) {
				char *entry;
				entry = menu_entry(debconf, language, defpkg);
				debconf->command(debconf, "SET",
						 MISSING_PROVIDE, entry, NULL);
				free(entry);
			} else
				/* TODO: How to figure out a default? */
				priority = "critical";
			debconf->command(debconf, "CAPB backup", NULL);
			debconf->command(debconf, "SUBST", MISSING_PROVIDE,
					"CHOICES", choices, NULL);
			debconf->command(debconf, "INPUT", priority, MISSING_PROVIDE,
					NULL);
			if (debconf->command(debconf, "GO", NULL) != 0)
				return 0;
			debconf->command(debconf, "CAPB", NULL);
			debconf->command(debconf, "GET", MISSING_PROVIDE, NULL);
			choice = strdup(debconf->value);
		}
		/* Go through the dependencies again */
		for (i = 0; p->depends[i] != 0; i++) {
			char *entry;
			if ((dep = p->depends[i]->ptr) == NULL)
				continue;
			if (!di_pkg_provides(dep, p))
				continue;
			entry = menu_entry(debconf, language, dep);
			if (!is_menu_item || strcmp(choice, entry) == 0) {
				free(entry);
				/* Ick. If we have a menu item it has to match the
				 * debconf choice, otherwise we configure all of
				 * the providing packages */
				if (!di_config_package(dep, satisfy_virtual, check_special))
					return 0;
				if (is_menu_item)
					break;
			} else
				free(entry);
		}
	}
	debconfclient_delete(debconf);
	free(choice);
	free(choices);
	free(language);
	/* It doesn't make sense to configure virtual packages,
	 * since they are, well, virtual. */
	p->status = installed;
	return 1;
}

static void update_language (void) {
	struct debconfclient *debconf;

	debconf = debconfclient_new();
	debconf->command(debconf, "GET", "debian-installer/language", NULL);
	if (*debconf->value != 0)
		setenv("LANGUAGE", debconf->value, 1);
	debconfclient_delete(debconf);
}

static int
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
	return 0;
}

int do_menu_item(struct package_t *p) {
	struct debconfclient *debconf;
	char *configcommand, *title;
	int ret = 0;

	di_logf("Menu item '%s' selected", p->package);

	debconf = debconfclient_new();
	asprintf(&title, "debian-installer/%s/title", p->package);
	if (debconf_settitle(debconf, title))
		di_logf("Unable to set title for %s.", p->package);
	free(title);
	debconfclient_delete(debconf);

	if (p->status == installed) {
		/* The menu item is already configured, so reconfigure it. */
		if (asprintf(&configcommand, "exec " DPKG_CONFIGURE_COMMAND " --force-configure %s", p->package) == -1) {
			return 0;
		}
                ret = SYSTEM(configcommand);
		free(configcommand);
		check_special(p);
                if (ret) {
                    di_logf("Reconfiguring '%s' failed with error code %d",
			    p->package, ret);
                }
                ret = !ret;
	}
	else if (p->status == unpacked || p->status == half_configured) {
		ret = di_config_package(p, satisfy_virtual, check_special);
	}

	return ret;
}

static char *debconf_priorities[] =
  {
    "low",
    "medium",
    "high",
    "critical"
  };

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
static void modify_debconf_priority (int raise_or_lower) {
	int pri;
	const char *template = "debconf/priority";
	struct debconfclient *debconf = debconfclient_new();
	debconf->command(debconf, "GET", template, NULL);
	if ( ! debconf->value )
		pri = 1;
	else
		for (pri = 0; pri < ARRAY_SIZE(debconf_priorities); ++pri) {
			if (0 == strcmp(debconf->value,
					debconf_priorities[pri]) )
				break;
		}
	if (raise_or_lower == LOWER)
		--pri;
	else if (raise_or_lower == RAISE)
		++pri;
	if (0 > pri)
		pri = 0;
	if (pri > default_priority)
		pri = default_priority;
	if (local_priority != pri) {
		di_logf("Modifying debconf priority limit from '%s' to '%s'",
			debconf->value ? debconf->value : "(null)",
			debconf_priorities[pri] ? debconf_priorities[pri] : "(null)");
		local_priority = pri;
	    
		debconf->command(debconf, "SET", template,
				 debconf_priorities[pri], NULL);
	}
	debconfclient_delete(debconf);

}
	
static void adjust_default_priority (void) {
	int pri;
	const char *template = "debconf/priority";
	struct debconfclient *debconf = debconfclient_new();
	debconf->command(debconf, "GET", template, NULL);	
	if ( ! debconf->value )
		pri = 1;
	else
	    for (pri = 0; pri < ARRAY_SIZE(debconf_priorities); ++pri) {
		if (0 == strcmp(debconf->value,
				debconf_priorities[pri]) )
		    break;
	    }
	if ( pri != local_priority ) {
		di_logf("Priority changed externally, setting main-menu default to '%s'",
			debconf_priorities[pri] ? debconf_priorities[pri] : "(null)");
		local_priority = pri;
		default_priority = pri;
	}
	debconfclient_delete(debconf);
}

int main (int argc, char **argv) {
	di_package *p;
        di_packages *packages;
        di_packages_allocator *allocator;
	int ret;

        debconf = debconfclient_new();
	
	/* This spawns a process that traps all stderr from the rest of
	 * main-menu and the programs it calls, storing it in STDERR_LOG. */
	intercept_stderr();
	
	/* Tell udpkg to shut up. */
	setenv("UDPKG_QUIET", "y", 1);

	packages = di_system_packages_status_read_file();
	while ((p=show_main_menu(packages))) {
		ret = do_menu_item(p);
		adjust_default_priority();
		if (!ret) {
			if (ret == 10)
				di_logf("Menu item '%s' succeeded but requested to be left unconfigured.", p->package); 
			else
				di_logf("Menu item '%s' failed.", p->package);
			/* Something went wrong.  Lower debconf
			   priority limit to try to give the user more
			   control over the situation. */
			modify_debconf_priority(LOWER);
		}
		else
			modify_debconf_priority(RAISE);
		
		/* Check for pending stderr in the stderr log, and
		 * display it in a nice debconf dialog. */
		/* XXX Pass in a title */
		display_stderr_log(p->package);

		di_list_free(packages, di_pkg_free);
		packages = di_status_read();

		/* tell cdebconf to save the database */
		kill(getppid(), SIGUSR1);
	}
	
	return(0);
}
