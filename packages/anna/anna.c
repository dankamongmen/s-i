#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "anna.h"

#define LOWMEM_STATUS_FILE "/var/lib/lowmem"
int lowmem=0;
char *choose_modules_question;

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
    di_package *package, *status_package, **package_array, *test_package;
    di_slist_node *node, *node1, *node2;
    int reverse_depend=0;
    bool kernel_packages_present = false;

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
        
    /* Go through the available packages to see if it contains at least
       one package that is valid for the subarchitecture and corresponds
       to the kernel version we are running */
    for (node = (*packages)->list.head; node; node = node->next) {
        package = node->data;
        
        if (!di_system_package_check_subarchitecture(package, subarchitecture))
            continue;
        if (((di_system_package *)package)->kernel_version) {
            if (running_kernel && strcmp(running_kernel, ((di_system_package *)package)->kernel_version) == 0) {
                kernel_packages_present = true;
                break;
            }
        }
    }

    if (!kernel_packages_present) {
        di_log(DI_LOG_LEVEL_WARNING, "no packages for kernel in archive");
        debconf_input(debconf, "critical", "anna/no_kernel_modules");
        if (debconf_go(debconf) == 30)
	    return 4; /* back to main menu */
        debconf_get(debconf, "anna/no_kernel_modules");
        if (strcmp(debconf->value, "false") == 0)
    	    return 4;
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

        if (package->type != di_package_type_real_package)
          continue;
        if (is_installed(package, status))
          continue;
        if (!di_system_package_check_subarchitecture(package, subarchitecture))
          continue;

	di_log (DI_LOG_LEVEL_DEBUG, "lowmem: %d, debconf status: %s", lowmem, debconf->value);
	
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
 
	if (lowmem > 1) {
	     if (package->priority == di_package_priority_standard
		 && ! ((di_system_package *)package)->installer_menu_item) {
		  /* get only packages which are not dependencies of other packages */
		  reverse_depend=0;
		  for (node1 = (*packages)->list.head; node1; node1 = node1->next) {
		       test_package = node1->data;
		       for (node2 = test_package->depends.head; node2; node2 = node2->next) {
			    di_package_dependency *d = node2->data;
			    if (d->ptr == package) {
				 reverse_depend=1;
			    }
		       }
		  }
		  if (reverse_depend == 0 && ! ((di_system_package *)package)->kernel_version) {
		       package->status_want = di_package_status_want_unknown;
		  }
		  package->priority = di_package_priority_optional;
	     }
	}

        if (package->priority >= di_package_priority_standard || is_queued(package))
        {
            package->status_want = di_package_status_want_install;
            di_log (DI_LOG_LEVEL_DEBUG, "install %s, priority >= standard", package->package);
        }
        else if (((di_system_package *)package)->installer_menu_item 
		 && package->status != di_package_status_installed) /* we don't want to see installed packages
								     * in choices list*/
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
    debconf_subst(debconf, choose_modules_question, "CHOICES", choices);
    if (lowmem < 2) {
	 debconf_input(debconf, "medium", choose_modules_question);
    }
    else {
	 debconf_input(debconf, "high", choose_modules_question);
    }
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

    debconf_get(debconf, choose_modules_question);
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
            if (!package->filename) {
                di_log(DI_LOG_LEVEL_ERROR, "no Filename field for %s, ignoring", package->package);
                continue;
            }
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
		    /* error handling may use a progress bar, so stop the current one */
                    debconf_progress_stop(debconf);
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
		    /* error handling may use a progress bar, so stop the current one */
                    debconf_progress_stop(debconf);
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

int get_lowmem_level () {
	int l;
	l=open(LOWMEM_STATUS_FILE, O_RDONLY);
	if (l) {
		char buf[2];
		read(l, buf, 1);
		close(l);
		return atoi(buf);
	}
	return 0;
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

    lowmem=get_lowmem_level();
    if (lowmem < 2) {
	choose_modules_question="anna/choose_modules";
    }
    else {
	choose_modules_question="anna/choose_modules_lowmem";
    }

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
