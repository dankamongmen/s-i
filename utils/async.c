/*
 * Usage: async  [-n] "program" title_template
 * Execute the command, cancelling it if cdebconf tells us to.
 * -n means the command doesn't do any output, so we put up a progress bar
 * ourselves.
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: async.c,v 1.2 2003/07/16 12:56:11 mckinstry Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <debian-installer.h>
#include <cdebconf/debconfclient.h>

#define MAX_LINE	1024	/* Max title size */

pid_t pid; // pid of the process we've started

void do_exit (void) ;
void start_progress_bar(char *template);
void stop_progress_bar();
static struct debconfclient *debconf = NULL;
int finish_up = 0;

/* FDs:
 * cstdin - stdin from cdebconf.
 * cstdout - stdout _to_ cdebconf
 */
int main (int argc, char **argv)
{
	int do_spinner = 0;
	char *cmd = NULL, *template = NULL;
	int ret = 0, r;
	fd_set rfds;
	sigset_t sigmask;
	int toconfig[2], fromconfig[2]; /* 0=read, 1=write */


	if (argc != 3 && argc != 4) {
		di_logf ("async: wrong number of parameters \n");
		exit (1);
	}
	if (argc == 3) {
		cmd = argv[1];
		template = argv[2];
	} else {
		if (strcmp (argv[1], "-n")) {
			di_logf("async: bad parameter '%s'\n", argv[1]);
			exit (1);
		}
		do_spinner = 1;
		cmd = argv[3];
		template = argv[4];
	}


	pipe(toconfig);
	pipe(fromconfig);
	
	switch (pid = fork())  {
	case -1:
		di_logf ("async: fork failed");
		exit (1);
		break;
	case 0:	
		if (do_spinner)
			start_progress_bar(template);

		// Ok, we set up the select; monitor stdin from 
		// cdebconf and the output from 
		while (!finish_up) {
			// FIXME
			r = pselect (3, &rfds, NULL, NULL, NULL, &sigmask);
			if (r < 0) {
				di_logf ("async: pselect error (%s) \n", strerror (errno));
				exit (1);
			}
			if (FD_ISSET(0 /*FIXME*/, &rfds))  // input from cdebconf. May only be BACK.  ?
				finish_up = 1;
			
		}

		if (do_spinner)
			stop_progress_bar();

		break;

	default:
		// Run the child. Pass the status back via the exit call
		exit (system (cmd));
	}
	exit (ret);	
}

/*
 * We've got a signal;  kill the process and 
 */
void do_exit (void)
{
	kill (pid, SIGKILL);
	printf ("PROGRESS STEP\n");
}

void chld_handler (int x)
{
	finish_up = 1;
}

void start_progress_bar (char *title_template)
{
	char buffer[MAX_LINE];
	// FIXME
	debconf = debconfclient_new();
	debconf->command (debconf, "CAPB", "backup", NULL);
	snprintf (buffer, MAX_LINE,"PROGRESS START 0 1 %s", title_template); 
	debconf->command (debconf, buffer, NULL);
	
}

void stop_progress_bar (void)
{
		// FIXME WRITEME
}


