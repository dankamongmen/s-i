/* stderr logging and display for main-menu. */

#include "stderr-log.h"
#include <cdebconf/debconfclient.h>
#include <debian-installer.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <assert.h>

/* The log file is opened anew for each error, because it may be removed in
 * between. Not efficient, but the idea is that stderr should not happen. */
void log_error (const char *s) {
	FILE *f;

	f = fopen(STDERR_LOG, "a");
	fprintf(f, s);
	fflush(f);
	fclose(f);
}

/* Fork a listener process, that listens to the stderr of the main-menu
 * and anything it runs. */
void intercept_stderr(void) {
	pid_t pid;
	int filedes[2];

	if (pipe(filedes) != 0) {
		perror("pipe");
		exit(1);
	}

	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(1);
	}

	if (pid) {
		close(filedes[0]);
		if (dup2(filedes[1], 2) == -1) {
			perror("dup2");
			exit(1);
		}
		return; /* go on with what main-menu does */
	}
	else {
		char buf[1024];
		FILE *f = fdopen(filedes[0], "r");

		close(filedes[1]);
		while ((fgets(buf, 1024, f))) {
			log_error(buf);
		}
		exit(0);
	}
}

void display_stderr_log(const char *package) {
	static struct debconfclient *debconf = NULL;
	static const char *titletemplate = "debian-installer/generic_error-title";
	static const char *template = "debian-installer/generic_error";
	FILE *f;

	assert(package);

	if (! debconf)
		debconf = debconfclient_new();
	
	if ((f = fopen(STDERR_LOG, "r"))) {
		int size = 0;
		char *p, *ret = NULL;

		if (feof(f))
			return;

		do {
			ret = realloc(ret, size + 128 + 1);
			assert(ret);
			if (! fgets(ret + size, 128, f)) {
				if (size == 0) {
					free(ret);
					return; /* eof with empty string */
				}
				else {
					ret[size]='\0';
					break;
				}
			}
			size = strlen(ret);
		} while (size > 0 && ! feof(f));

		di_log(DI_LOG_LEVEL_WARNING, "Package '%s' printed to stderr, size=%d.", package, size);
		
		/* remove newlines, as they screw up the debconf
		 * protocol. Which might one day be fixed.. */
		while ((p = strchr(ret, '\n')))
			p[0]=' ';
	
		debconf_subst(debconf, titletemplate, "PACKAGE", package);
		debconf_settitle(debconf, titletemplate);
		debconf_subst(debconf, template, "PACKAGE", package);
		debconf_subst(debconf, template, "ERROR", ret);
		debconf_input(debconf, "high", template);
		debconf_go(debconf);

		fclose(f);
		unlink(STDERR_LOG);
	}
}
