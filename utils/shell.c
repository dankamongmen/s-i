/* 
  shell.c - interactive shell for debian-installer
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cdebconf/debconfclient.h>


#define  CLEAR "[H[J"

int
shell_main (int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
  struct debconfclient *client;
  client = debconfclient_new ();

  debconf_input (client, "medium", "di-utils-shell/do-shell");
  debconf_go (client);

  if ((dup2 (DEBCONF_OLD_STDIN_FD, 0) == -1)
      || (dup2 (DEBCONF_OLD_STDOUT_FD, 1) == -1)
      || (dup2 (DEBCONF_OLD_STDOUT_FD, 2) == -1))
      return -1;

  chdir ("/");
  execl ("/bin/sh", "/bin/sh", NULL);

  return 0;
}
