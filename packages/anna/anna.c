#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include "anna.h"

struct debconfclient *debconf = NULL;
static char *running_kernel = NULL, *subarchitecture;

static int
choose_retriever(di_packages *status, di_packages **packages __attribute__((unused)), di_packages_allocator **packages_allocator __attribute__((unused)))
{
    char *choices;
    di_package **retrievers;

    retrievers = get_retriever_packages(status);
    if (!retrievers)
        di_log(DI_LOG_LEVEL_WARNING, "can't find any retrievers");
    choices = list_to_choices(retrievers);
    if (!choices)
        di_log(DI_LOG_LEVEL_ERROR, "can't build choices");
    debconf_fget(debconf, ANNA_RETRIEVER, "seen");
    if (strcmp(debconf->value, "false") == 0) {
        const char *retriever = get_default_retriever(choices);
        char buf[200];
        di_package *p;

        if (retriever && (p = di_packages_get_package(status, retriever, 0))) {
            package_to_choice(p, buf, 200);
	    set_retriever(buf);
        }
    }

    debconf_fset(debconf, ANNA_RETRIEVER, "seen", "false");
    debconf_subst(debconf, ANNA_RETRIEVER, "CHOICES", choices);
    debconf_input(debconf, "medium", ANNA_RETRIEVER);

    di_free(retrievers);
    di_free(choices);

    return 0;
}

static int
is_queued(di_package *package)
{
    FILE *fp;
    char buf[1024];

    if ((fp = fopen("/var/lib/anna-install/queue", "r")) != NULL) {
	while (fgets(buf, sizeof(buf), fp)) {
	    buf[strlen(buf) - 1] = '\0';

	    if (strcmp(buf, package->package) == 0) {
		fclose(fp);
		return 1;
	    }
	}
	fclose(fp);
    }

    return 0;
}

static int
choose_modules(di_packages *status, di_packages **packages, di_packages_allocator **packages_allocator)
{
    char *choices;
    int package_count = 0;
    di_package *package, *status_package, **package_array;
    di_slist_node *node, *node1;

    config_retriever();

    *packages_allocator = di_system_packages_allocator_alloc();
    *packages = get_packages(*packages_allocator);

    while (*packages == NULL) {
	int status=retriever_handle_error("packages");
        di_log(DI_LOG_LEVEL_WARNING, "bad d-i Packages file");
	if (status != 1) {
	    /* Failed to handle error. */
	    return 4;
	}
	else {
            /* Error handled, retry. */
            *packages_allocator = di_system_packages_allocator_alloc();
            *packages = get_packages(*packages_allocator);
	}
    }

    for (node = status->list.head; node; node = node->next) {
        status_package = node->data;
        package = di_packages_get_package(*packages, status_package->package, 0);
        if (!package)
            continue;
        package->status = status_package->status;
        if (status_package->status == di_package_status_unpacked || status_package->status == di_package_status_installed) {
            for (node1 = package->depends.head; node1; node1 = node1->next) {
                di_package_dependency *d = node1->data;
                if (d->type == di_package_dependency_type_reverse_enhances)
                {
                    package->status_want = di_package_status_want_install;
                    di_log (DI_LOG_LEVEL_DEBUG, "install %s, enhances installed packages %s", package->package, status_package->package);
                }
            }
        }
    }

    for (node = (*packages)->list.head; node; node = node->next) {
        package = node->data;

        package->status_want = di_package_status_want_deinstall;

        if (package->status_want == di_package_status_want_install)
          continue;
        if (package->type != di_package_type_real_package)
          continue;
        if (is_installed(package, status))
          continue;
        if (!di_system_package_check_subarchitecture(package, subarchitecture))
          continue;

        if (((di_system_package *)package)->kernel_version)
        {
          if (running_kernel && strcmp(running_kernel, ((di_system_package *)package)->kernel_version) == 0)
          {
              package->status_want = di_package_status_want_unknown;
              di_log (DI_LOG_LEVEL_DEBUG, "ask for %s, matches kernel", package->package);
          }
          else
            continue;
        }
        if (package->priority >= di_package_priority_standard || is_queued(package))
        {
            package->status_want = di_package_status_want_install;
            di_log (DI_LOG_LEVEL_DEBUG, "install %s, priority >= standard", package->package);
        }
        else if (((di_system_package *)package)->installer_menu_item)
        {
            package->status_want = di_package_status_want_unknown;
            di_log (DI_LOG_LEVEL_DEBUG, "ask for %s, is installer item", package->package);
        }
    }

    /* Include packages in udeb_include */
    take_includes(*packages);

    /* Drop packages in udeb_exclude */
    drop_excludes(*packages);

    di_system_packages_resolve_dependencies_mark_anna(*packages, subarchitecture, running_kernel);

    /* Slight over-allocation, but who cares */
    package_array = di_new0(di_package *, di_hash_table_size((*packages)->table));
    /* Now build the asklist, figuring out which packages have been
     * pulled into instlist */
    for (node = (*packages)->list.head; node; node = node->next) {
        package = node->data;
        if (package->status_want == di_package_status_want_unknown)
            package_array[package_count++] = package;
    }

    qsort(package_array, package_count, sizeof(di_package *), package_array_compare);
    choices = list_to_choices(package_array);
    debconf_fset(debconf, ANNA_CHOOSE_MODULES, "seen", "false");
    debconf_subst(debconf, ANNA_CHOOSE_MODULES, "CHOICES", choices);
    debconf_input(debconf, "medium", ANNA_CHOOSE_MODULES);

    di_free(choices);
    di_free(package_array);

    return 0;
}

void resume_progress_bar (int progress_step, int pkg_count, di_package *package) {
    debconf_progress_start(debconf, 0, 2*pkg_count,  "anna/progress_title");
    debconf_progress_set(debconf, progress_step);
    debconf_subst(debconf, "anna/progress_step_retr", "PACKAGE", package->package);
    debconf_subst(debconf, "anna/progress_step_inst", "PACKAGE", package->package);
    debconf_progress_info(debconf, "anna/progress_step_retr");
}

static int
install_modules(di_packages *status, di_packages *packages, di_packages_allocator *status_allocator __attribute__ ((unused)))
{
    di_slist_node *node;
    di_package *package;
    char *f, *fp, *dest_file;
    int ret = 0, pkg_count = 0;
    int progress_step=0;

    debconf_get(debconf, ANNA_CHOOSE_MODULES);
    if (debconf->value != NULL) {
        char *choices = debconf->value;

        for (node = packages->list.head; node; node = node->next) {
            package = node->data;
            /* Not very safe, but at least easy ;) */
            if (strstr(choices, package->package) != NULL)
                package->status_want = di_package_status_want_install;
        }
    }

    di_system_packages_resolve_dependencies_mark_anna(packages, subarchitecture, running_kernel);

    for (node = packages->list.head; node; node = node->next) {
        package = node->data;
        if (package->status_want == di_package_status_want_install && !is_installed(package, status))
            pkg_count++;
        else
            package->status_want = di_package_status_want_unknown;
    }
    // Short-circuit if there's no packages to install
    if (pkg_count <= 0)
        return 0;
    debconf_progress_start(debconf, 0, 2*pkg_count,  "anna/progress_title");
    
    for (node = packages->list.head; node; node = node->next) {
        package = node->data;
        if (package->type == di_package_type_real_package && package->status_want == di_package_status_want_install) {
            /*
             * Come up with a destination filename.. let's use
             * the filename we're getting, minus any path
             * component.
             */
            for(f = fp = package->filename; *fp != 0; fp++)
                if (*fp == '/')
                    f = ++fp;
            if (asprintf(&dest_file, "%s/%s", DOWNLOAD_DIR, f) == -1) 
                return 5;

            debconf_subst(debconf, "anna/progress_step_retr", "PACKAGE", package->package);
            debconf_subst(debconf, "anna/progress_step_inst", "PACKAGE", package->package);
            debconf_progress_info(debconf, "anna/progress_step_retr");
	    for (;;) {
                if (get_package(package, dest_file)) {
                    di_log(DI_LOG_LEVEL_WARNING, "get_package failed");
                    debconf_progress_stop(debconf); /* error handling may use a progress bar, so stop the current one */
		    if (retriever_handle_error("retrieve") != 1) {
		    	/* Failed to handle error. */
                        free(dest_file);
                        ret = 6;
                        goto OUT;
		    }
		    else {
			/* Handled error, retry. */
			resume_progress_bar(progress_step, pkg_count, package);
		        continue;
		    }
                }
                
		if (! md5sum(package->md5sum, dest_file)) {
                    di_log(DI_LOG_LEVEL_WARNING, "bad md5sum");
                    debconf_progress_stop(debconf); /* error handling may use a progress bar, so stop the current one */
		    if (retriever_handle_error("retrieve") != 1) {
		        /* Failed to handle error. */
                        unlink(dest_file);
                        free(dest_file);
                        ret = 7;
			goto OUT;
		    }
		    else {
		        /* Handled error, retry. */
			resume_progress_bar(progress_step, pkg_count, package);
			continue;
		    }
                }

		break;
            }
            debconf_progress_step(debconf, 1);
	    progress_step++;
            debconf_progress_info(debconf, "anna/progress_step_inst");
#ifdef LIBDI_SYSTEM_DPKG
            if (di_system_dpkg_package_unpack(status, package->package, dest_file, status_allocator)) {
#else
            if (!unpack_package(dest_file)) {
#endif
                debconf_progress_stop(debconf);
                debconf_subst(debconf, "anna/install_failed", "PACKAGE", package->package);
                debconf_input(debconf, "critical", "anna/install_failed");
                debconf_go(debconf);
                unlink(dest_file);
                free(dest_file);
                ret = 8;
                break;
            }
	    if (!((di_system_package *)package)->installer_menu_item)
		if (!configure_package(package->package)) {
		    debconf_progress_stop(debconf);
		    debconf_subst(debconf, "anna/install_failed", "PACKAGE",
				  package->package);
		    debconf_input(debconf, "critical", "anna/install_failed");
		    debconf_go(debconf);
		    unlink(dest_file);
		    free(dest_file);
		    ret = 8;
		    break;
		}
            unlink(dest_file);
            free(dest_file);
            debconf_progress_step(debconf, 1);
	    progress_step++;
        }
    }


    debconf_progress_stop(debconf);
    
OUT:
    return ret;
}

#ifndef TEST
int
main(int argc, char **argv)
{
    int ret, state;
    int firststate = 0;
    int (*states[])(di_packages *status, di_packages **packages, di_packages_allocator **packages_allocator) = {
        choose_retriever,
        choose_modules,
        NULL,
    };
    di_packages *packages = NULL, *status;
    di_packages_allocator *packages_allocator = NULL, *status_allocator;
    struct utsname uts;

    debconf = debconfclient_new();
    debconf_capb(debconf, "backup");

    di_system_init("anna");

    if (debconf_get(debconf, "debian-installer/kernel/subarchitecture"))
        subarchitecture = strdup("generic");
    else
        subarchitecture = strdup(debconf->value);

    if (uname(&uts) == 0)
        running_kernel = strdup(uts.release);

    status_allocator = di_system_packages_allocator_alloc();
    status = di_system_packages_status_read_file(DI_SYSTEM_DPKG_STATUSFILE, status_allocator);

    di_package **retrievers_before = get_retriever_packages(status);

    if (argc > 1) {
	    set_retriever(argv[1]);
	    firststate=1; /* skip manual setting and use the supplied retriever */
    }
    
    state=firststate;
    while (state >= firststate && states[state] != NULL) {
        ret = states[state](status, &packages, &packages_allocator);
        if (ret != 0)
            state = -1;
        else if (debconf_go(debconf) == 0)
            state++;
        else
        {
            if (packages)
            {
                di_packages_free(packages);
                di_packages_allocator_free(packages_allocator);
                packages = NULL;
                packages_allocator = NULL;
            }
            state--;
        }
    }
    if (state < firststate)
        ret = 10; /* back to the menu */
    else
    {
        ret = install_modules(status, packages, status_allocator);
#ifdef LIBDI_SYSTEM_DPKG
        di_system_packages_status_write_file(status, DI_SYSTEM_DPKG_STATUSFILE);
#endif
    }
    if (!ret && argc > 1) {
        di_package **retrievers_after = get_retriever_packages(status);
        if (new_retrievers(retrievers_before, retrievers_after))
            ret = 10;
    }

    cleanup();
    return ret;
}
#else
int
main()
{
    char name[] = "foo-modules-2.4.22-1-di\0aaaaaaa";
    di_package package;
    package.package = name;
    assert(strcmp(udeb_kernel_version(&package), "2.4.22-1") == 0);
    return 0;
}
#endif
