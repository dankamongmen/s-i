#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include "anna.h"

struct debconfclient *debconf = NULL;

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
            debconf_set(debconf, ANNA_RETRIEVER, buf);
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
choose_modules(di_packages *status, di_packages **packages, di_packages_allocator **packages_allocator)
{
    char *choices, *package_kernel, *running_kernel = NULL, *subarchitecture;
    int package_count = 0;
    di_package *package, *status_package, **package_array;
    di_slist_node *node, *node1;
    struct utsname uts;

    config_retriever();
    if (uname(&uts) == 0)
        running_kernel = uts.release;

    *packages_allocator = di_system_packages_allocator_alloc();
    *packages = get_packages(*packages_allocator);

    if (*packages == NULL) {
        debconf_input(debconf, "critical", ANNA_NO_MODULES);
        debconf_go(debconf);
        return 4;
    }

    get_initial_package_list(*packages);

    for (node = status->list.head; node; node = node->next) {
        status_package = node->data;
        if (status_package->status == di_package_status_unpacked || status_package->status == di_package_status_installed) {
            package = di_packages_get_package(*packages, status_package->package, 0);
            if (!package)
                continue;
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

#if 0
    if (debconf_get(debconf, "debian-installer/kernel/subarchitecture"))
      subarchitecture = strdup("generic");
    else
      subarchitecture = strdup(debconf->value);
#endif

    for (node = (*packages)->list.head; node; node = node->next) {
        package = node->data;
        package_kernel = udeb_kernel_version(package);

        package->status_want = di_package_status_want_deinstall;

        if (package->status_want == di_package_status_want_install)
          continue;
        if (package->type != di_package_type_real_package)
          continue;
        if (is_installed(package, status))
          continue;
#if 0
        if (!di_system_package_check_subarchitecture(package, subarchitecture))
          continue;
#endif

        if (running_kernel && package_kernel && strcmp(running_kernel, package_kernel) == 0)
        {
            package->status_want = di_package_status_want_unknown;
            di_log (DI_LOG_LEVEL_DEBUG, "ask for %s, matches kernel", package->package);
        }
	else if (package_kernel)
	  continue;
        if (package->priority >= di_package_priority_standard)
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

    /* Drop files in udeb_exclude */
    drop_excludes(*packages);

    di_packages_resolve_dependencies_mark(*packages);

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

static int
install_modules(di_packages *status, di_packages *packages, di_packages_allocator *status_allocator __attribute__ ((unused)))
{
    di_slist_node *node;
    di_package *package;
    char *f, *fp, *dest_file;
    int ret = 0, pkg_count = 0;

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

    di_packages_resolve_dependencies_mark(packages);

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
            if (get_package(package, dest_file)) {
                debconf_progress_stop(debconf);
                debconf_subst(debconf, "anna/retrieve_failed", "PACKAGE", package->package);
                debconf_input(debconf, "critical", "anna/retrieve_failed");
                debconf_go(debconf);
                free(dest_file);
                ret = 6;
                break;
            }
            if (!md5sum(package->md5sum, dest_file)) {
                debconf_progress_stop(debconf);
                debconf_subst(debconf, "anna/md5sum_failed", "PACKAGE", package->package);
                debconf_input(debconf, "critical", "anna/md5sum_failed");
                debconf_go(debconf);
                unlink(dest_file);
                free(dest_file);
                ret = 7;
                break;
            }
            debconf_progress_step(debconf, 1);
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
            unlink(dest_file);
            free(dest_file);
            debconf_progress_step(debconf, 1);
        }
    }

    debconf_progress_stop(debconf);

    return ret;
}

int
main()
{
    int ret, state = 0;
    int (*states[])(di_packages *status, di_packages **packages, di_packages_allocator **packages_allocator) = {
        choose_retriever,
        choose_modules,
        NULL,
    };
    di_packages *packages = NULL, *status;
    di_packages_allocator *packages_allocator = NULL, *status_allocator;

    debconf = debconfclient_new();
    debconf_capb(debconf, "backup");

    di_system_init("anna");

    status_allocator = di_system_packages_allocator_alloc();
    status = di_system_packages_status_read_file(DI_SYSTEM_DPKG_STATUSFILE, status_allocator);

    di_package **retrievers_before = get_retriever_packages(status);

    while (state >= 0 && states[state] != NULL) {
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
    if (state < 0)
        ret = 10; /* back to the menu */
    else
    {
        ret = install_modules(status, packages, status_allocator);
#ifdef LIBDI_SYSTEM_DPKG
        di_system_packages_status_write_file(status, DI_SYSTEM_DPKG_STATUSFILE);
#endif
    }
    if (!ret) {
        di_package **retrievers_after = get_retriever_packages(status);
        if (new_retrievers(retrievers_before, retrievers_after))
            ret = 10;
    }

    cleanup();
    return ret;
}
