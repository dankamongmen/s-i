#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cdebconf/debconfclient.h>
#include <debian-installer.h>
#include "anna.h"

static struct debconfclient *debconf = NULL;


static int
choose_retriever(void)
{
    struct linkedlist_t *ret_pkgs;
    char *ret_choices;

    ret_pkgs = get_retriever_packages();
    if (ret_pkgs == NULL)
        return 1;
    ret_choices = get_retriever_choices(ret_pkgs);
    if (ret_choices[0] == '\0')
        return 2;
    debconf->command(debconf, "FGET", ANNA_RETRIEVER, "seen", NULL);
    if (strcmp(debconf->value, "false") == 0) {
        char *defval = (char *)get_default_retriever(ret_choices);
        struct package_t *p;

        if (defval != NULL && (p = di_pkg_find(ret_pkgs, defval))) {
            asprintf(&defval, "%s: %s", p->package, p->description);
            debconf->command(debconf, "SET", ANNA_RETRIEVER, defval, NULL);
            free(defval);
        }
    }
    di_list_free(ret_pkgs, di_pkg_free);

    debconf->command(debconf, "FSET", ANNA_RETRIEVER, "seen", "false", NULL);
    debconf->command(debconf, "SUBST", ANNA_RETRIEVER, "CHOICES", ret_choices, NULL);
    debconf->command(debconf, "INPUT medium", ANNA_RETRIEVER, NULL);

    free(ret_choices);

    return 0;
}


/* Ick */
static struct linkedlist_t *instlist, *asklist;

static int
choose_modules(void)
{
    struct linkedlist_t *pkglist, *tmplist, *status_p;
    struct list_node *node, *prev, *next;
    struct package_t *p;
    FILE *fp;
    char *choices;

    pkglist = get_packages();
    if (pkglist == NULL || pkglist->head == NULL) {
        debconf->command(debconf, "INPUT critical", ANNA_NO_MODULES, NULL);
        debconf->command(debconf, "GO", NULL);
        free(pkglist);
        return 4;
    }
    di_pkg_resolve_deps(pkglist);
    /* OK. We have a list of packages. What we want to do is partition it in
     * two parts. One with packages that will be installed and one with
     * packages that *might* be installed */
    fp = fopen(STATUS_FILE, "r");
    if (fp == NULL)
        return 5;
    status_p = di_pkg_parse(fp);
    fclose(fp);

    /* Figure out which packages we definitely want to install */
    instlist = malloc(sizeof(struct linkedlist_t));
    instlist->head = instlist->tail = NULL;
    prev = NULL;
    for (node = pkglist->head; node != NULL; node = next) {
        next = node->next;
        p = (struct package_t *)node->data;
        if (p->filename == NULL || is_installed(p, status_p)) {
            /* Packages without filenames(!) or packages already installed
             * will be brutally skipped. */
            if (prev)
                prev->next = next;
            else
                pkglist->head = next;
            /* FIXME: When is it safe to free p? */
            free(node);
            continue;
        }
        if (p->priority >= standard) {
            /* These packages will automatically be installed */
            if (prev != NULL)
                prev->next = next;
            else
                pkglist->head = next;
            if (instlist->tail == NULL) {
                instlist->head = node;
                instlist->tail = node;
            } else {
                instlist->tail->next = node;
                instlist->tail = node;
            }
            node->next = NULL;
            continue;
        }
        prev = node;
    }
    /* Pull in dependencies */
    tmplist = di_pkg_toposort_list(instlist);
    /* Free up the memory used by the nodes in the old instlist */
    //di_list_free(instlist, free); /* This actually causes memory corruption...dammit */
    di_list_free(status_p, di_pkg_free);
    instlist = tmplist;

    /* Now build the asklist, figuring out which packages have been
     * pulled into instlist */
    prev = NULL;
    asklist = pkglist;
    for (node = asklist->head; node != NULL; node = next) {
        next = node->next;
        p = (struct package_t *)node->data;
        if (di_pkg_find(instlist, p->package) != NULL) {
            /* We found it in instlist, remove it here */
            if (prev != NULL)
                prev->next = next;
            else
                asklist->head = next;
            free(node);
            continue;
        }
        prev = node;
    }
    choices = list_to_choices(asklist);
    debconf->command(debconf, "FSET", ANNA_CHOOSE_MODULES, "seen", "false", NULL);
    debconf->command(debconf, "SUBST", ANNA_CHOOSE_MODULES, "CHOICES", choices, NULL);
    debconf->command(debconf, "INPUT medium", ANNA_CHOOSE_MODULES, NULL);
    free(choices);

    return 0;
}

static int
install_modules(void)
{
    FILE *sfp;
    struct linkedlist_t *tmplist, *status_p;
    struct list_node *node;
    struct package_t *p;
    char *f, *fp, *dest_file;
    int ret = 0, pkg_count = 0;

    sfp = fopen(STATUS_FILE, "r");
    if (sfp == NULL)
        return 5;
    status_p = di_pkg_parse(sfp);
    fclose(sfp);
    debconf->command(debconf, "GET", ANNA_CHOOSE_MODULES, NULL);
    if (debconf->value != NULL) {
        char *choices = debconf->value;
        struct list_node *prev = NULL, *next;

        for (node = asklist->head; node != NULL; node = next) {
            next = node->next;
            p = (struct package_t *)node->data;
            /* Not very safe, but at least easy ;) */
            if (strstr(choices, p->package) != NULL) {
                if (prev != NULL)
                    prev->next = next;
                else
                    asklist->head = next;
                if (instlist->tail == NULL) {
                    instlist->head = node;
                    instlist->tail = node;
                } else {
                    instlist->tail->next = node;
                    instlist->tail = node;
                }
                node->next = NULL;
                continue;
            }
            prev = node;
        }
    }
    /* Pull in dependencies again since we might have added packages */
    di_pkg_resolve_deps(instlist);
    tmplist = di_pkg_toposort_list(instlist);
    /* Free some memory. Note, we don't know which of the nodes inside
       the asklist we can free. sigh. */
    free(asklist);
    instlist = tmplist;

    for (node = instlist->head; node != NULL; node = node->next) {
        p = (struct package_t *)node->data;
        if (p->filename != NULL && !is_installed(p, status_p))
            pkg_count++;
    }
    // Short-circuit if there's no packages to install
    if (pkg_count <= 0)
        return 0;
    debconf->commandf(debconf, "PROGRESS START 0 %d anna/progress_title", 2*pkg_count);
    for (node = instlist->head; node != NULL; node = node->next) {
        p = (struct package_t *)node->data;
        if (p->filename != NULL && !is_installed(p, status_p)) {
            /*
             * Come up with a destination filename.. let's use
             * the filename we're getting, minus any path
             * component.
             */
            for(f = fp = p->filename; *fp != 0; fp++)
                if (*fp == '/')
                    f = ++fp;
            asprintf(&dest_file, "%s/%s", DOWNLOAD_DIR, f);

            debconf->command(debconf, "SUBST", "anna/progress_step_retr", "PACKAGE", p->package, NULL);
            debconf->command(debconf, "SUBST", "anna/progress_step_inst", "PACKAGE", p->package, NULL);
            debconf->commandf(debconf, "PROGRESS STEP 1 anna/progress_step_retr");
            if (!get_package(p, dest_file)) {
                debconf->command(debconf, "PROGRESS STOP", NULL);
                debconf->command(debconf, "SUBST", "anna/retrieve_failed", "PACKAGE", p->package, NULL);
                debconf->command(debconf, "INPUT critical", "anna/retrieve_failed", NULL);
                debconf->command(debconf, "GO", NULL);
                free(dest_file);
                ret = 6;
                break;
            }
            if (!md5sum(p->md5sum, dest_file)) {
                debconf->command(debconf, "PROGRESS STOP", NULL);
                debconf->command(debconf, "SUBST", "anna/md5sum_failed", "PACKAGE", p->package, NULL);
                debconf->command(debconf, "INPUT critical", "anna/md5sum_failed", NULL);
                debconf->command(debconf, "GO", NULL);
                unlink(dest_file);
                free(dest_file);
                ret = 7;
                break;
            }
            debconf->commandf(debconf, "PROGRESS STEP 1 anna/progress_step_inst");
            if (!unpack_package(dest_file)) {
                debconf->command(debconf, "PROGRESS STOP", NULL);
                debconf->command(debconf, "SUBST", "anna/install_failed", "PACKAGE", p->package, NULL);
                debconf->command(debconf, "INPUT critical", "anna/install_failed", NULL);
                debconf->command(debconf, "GO", NULL);
                unlink(dest_file);
                free(dest_file);
                ret = 8;
                break;
            }
            unlink(dest_file);
            free(dest_file);
        }
    }

    debconf->command(debconf, "PROGRESS STOP", NULL);

    di_list_free(instlist, di_pkg_free);
    di_list_free(status_p, di_pkg_free);

    return ret;
}

int
main()
{
    int ret, state = 0;
    int (*states[])(void) = {
        choose_retriever,
        choose_modules,
        NULL,
    };

    debconf = debconfclient_new();
    debconf->command(debconf, "CAPB", "backup", NULL);
    while (state >= 0 && states[state] != NULL) {
        ret = states[state]();
        if (ret != 0)
            state = -1;
        else if (debconf->command(debconf, "GO", NULL) == 0)
            state++;
        else
            state--;
    }
    if (state < 0)
        ret = 10; /* back to the menu */
    else
        ret = install_modules();
    cleanup();
    return ret;
}
