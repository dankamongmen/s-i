/* 
  shell.c - interactive shell for debian-installer
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cdebconf/debconfclient.h>


#define  CLEAR "[H[J"

int
main ()
{
  struct debconfclient *client;
  client = debconfclient_new ();

  client->command (client, "input", "medium", "di-utils-shell/do-shell",
		   NULL);
  client->command (client, "go", NULL);

  if ((dup2 (DEBCONF_OLD_STDIN_FD, 0) == -1)
      || (dup2 (DEBCONF_OLD_STDOUT_FD, 1) == -1))
    {
      return -1;
    }

  chdir ("/");
  execl ("/bin/sh", "/bin/sh", NULL);

  return 0;
}
