/*
 * anna's not nearly apt, but for the Debian installer, it will do. 
 *
 * anna is Copyright (C) 2000 by Joey Hess, under the terms of the GPL.
 * Apologetically dedicated to my sister, Anna.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cdebconf/debconfclient.h>
#include "anna.h"

static int
is_installed(struct package_t *package, struct package_t *installed)
{
	struct package_t *q;
	struct version_t pv, qv;

	/* If we don't understand the version number, we play safe
	 * and assume we should install it */
	if (package->version == NULL || !di_parse_version(&pv, package->version))
		return 0;
	for (q = installed; q != NULL; q = q->next) {
		if (strcmp(package->package, q->package) == 0) {
			if (q->version == NULL || !di_parse_version(&qv, q->version))
				return 0;
			else
				return (di_compare_version(&pv, &qv) <= 0);
		}
	}
	return 0;
}

/*
 * This function takes a linked list of available packages, decides which
 * are worth installing, creates a linked list of those, and returns it.
 *
 * - Don't install packages that are already installed
 * - Ask for which packages with priority below standard to install
 */
struct package_t *select_packages (struct package_t *packages) {
	struct package_t *p, *prev = NULL, *q;
	struct package_t *status_p;
	struct package_t *lowpri_p = NULL, *lowpri_last = NULL;
	FILE *fp;

	fp = fopen(STATUS_FILE, "r");
	if (fp == NULL)
		return NULL;
	status_p = di_pkg_parse(fp);
	fclose(fp);

        for (p = packages; p; p = q)
        {
		q = p->next;
		if (is_installed(p, status_p)) {
                        if (prev)
                                prev->next = p->next;
                        else
                                packages = p->next;
                        continue;
                } else if (p->priority < standard) {
			/* Unlink these packages temporarily */
			if (prev)
				prev->next = p->next;
			else
				packages = p->next;
			if (lowpri_last == NULL) {
				lowpri_p = p;
				lowpri_last = p;
				p->next = NULL;
			} else {
				lowpri_last->next = p;
				p->next = NULL;
				lowpri_last = p;
			}
			continue;
		}
                prev = p;
        }
	if (lowpri_p != NULL) {
		struct debconfclient *debconf;
		char *choices;
		char *tmp;
		size_t choices_size = 1;

		choices = malloc(choices_size);
		choices[0] = '\0';
		for (p = lowpri_p; p != NULL; p = p->next) {
			choices_size += strlen(p->package) + 2 + strlen(p->description) + 2;
			choices = realloc(choices, choices_size);
			strcat(choices, p->package);
			strcat(choices, ": ");
			strcat(choices, p->description);
			strcat(choices, ", ");
		}
		if (choices_size >= 3)
			choices[choices_size-3] = '\0';
		debconf = debconfclient_new();
		debconf->command(debconf, "FSET", ANNA_CHOOSE_LOWPRI_PACKAGES,
				"seen", "false", NULL);
		debconf->command(debconf, "SUBST", ANNA_CHOOSE_LOWPRI_PACKAGES,
				"CHOICES", choices, NULL);
		debconf->command(debconf, "INPUT medium", ANNA_CHOOSE_LOWPRI_PACKAGES,
				NULL);
		debconf->command(debconf, "GO", NULL);
		free(choices);
		debconf->command(debconf, "GET", ANNA_CHOOSE_LOWPRI_PACKAGES, NULL);
		if (debconf->value != NULL) {
			/* This is probably not the best way to do it,
			 * but I'm feeling lazy. Feel free to improve it. */
			for (p = lowpri_p; p != NULL; p = q) {
				q = p->next;
				asprintf(&tmp, "%s: %s", p->package, p->description);
				if (tmp != NULL && strstr(debconf->value, tmp) != NULL) {
					p->next = packages;
					packages = p;
				}
				free(tmp);
			}
		}
	}
	return packages;
}

/* Calls udpkg to unpack a package. */
int unpack_package (char *pkgfile) {
	char *command;

	asprintf(&command, "%s %s", DPKG_UNPACK_COMMAND, pkgfile);
	return ! system(command);
}

/* check whether the md5sum of file matches sum.  if they don't,
 * return 0.
 */

int md5sum(char* sum, char *file) {
        int io[2];
        int pid;
        char line[1024];
	/* Trivially true if the Packages file doesn't have md5sum lines */
	if (sum == NULL)
		return 1;
        pipe(io);
        pid = fork();
        if (pid == 0) {
                /* child */
                dup2(io[1],1);
                execl("/usr/bin/md5sum","/usr/bin/md5sum",file,0);
        }
        wait(NULL);
        read(io[0],&line,1023);
        line[1023] = '\0';
        if (strlen(line) < 32) {
                /* not a success, return */
                return 0;
        }
        line[32] = '\0';
        /* line now contains just the md5sum */
        return ! strcmp(line,sum);
}

/*
 * Retrives and unpacks each package in the linked list in turn.
 * 
 * Returns false on failure, and aborts the operation.
 */
int install_packages (struct package_t *packages) {
	struct package_t *p;
	char *f, *fp, *dest_file;
	char *emsg;
	int ret = 1;

	for (p=packages; p; p=p->next) {
		if (p->filename) {
			/*
			 * Come up with a destination filename.. let's use
			 * the filename we're getting, minus any path
			 * component.
			 */
			for(f = fp = p->filename; *fp != 0; fp++)
				if (*fp == '/')
					f = ++fp;
			asprintf(&dest_file, "%s/%s", DOWNLOAD_DIR, f);

			if (! get_package(p, dest_file)) {
				asprintf(&emsg, "anna: error getting %s!\n",
					p->filename);
				di_log(emsg);
				free(emsg);
				ret = 0;
				break;
			} else if (! md5sum(p->md5sum, dest_file)) {
				asprintf(&emsg, "anna: md5sum mismatch on %s!\n",
					p->filename);
				di_log(emsg);
				free(emsg);
				unlink(dest_file);
				ret = 0;
				break;
			} else if (! unpack_package(dest_file)) {
				unlink(dest_file);
				ret = 0;
				break;
			}
			unlink(dest_file);
			free(dest_file);
		}
	}

	cleanup();

	return ret;
}

int main (int argc, char **argv) {
	/* Tell udpkg to shut up. */
	setenv("UDPKG_QUIET", "y", 1);
	
	return ! install_packages(select_packages(get_packages()));
}
