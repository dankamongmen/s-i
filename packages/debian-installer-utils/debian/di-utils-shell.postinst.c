/* Yes, that's right, a compiled postinst. */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <debian-installer.h>
#include <cdebconf/debconfclient.h>

int main () {
	struct debconfclient *client;
	client = debconfclient_new();

	debconf_capb(client, "backup");
	debconf_fset(client, "di-utils-shell/do-shell", "seen", "false");
	debconf_input(client, "high", "di-utils-shell/do-shell");
	if (debconf_go(client) == 30) {
		exit(10);
	}

	/* 
	 * To run the shell, we must first close the cdebconf file
	 * descriptors. That's why this postinst is a C program in
	 * the first place.
	 */
	if ((dup2(DEBCONF_OLD_STDIN_FD, 0) == -1) ||
	    (dup2(DEBCONF_OLD_STDOUT_FD, 1) == -1) ||
	    (dup2(DEBCONF_OLD_STDOUT_FD, 2) == -1))
		exit(1);

	/*  clear screen and reset background color, if possible */
	di_exec_shell("/usr/bin/clear");

	chdir("/");
	system("/bin/sh");
	exit(0);
}
