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

  if ((freopen ("/dev/console", "r", stdin) == NULL)
      || (freopen ("/dev/console", "w", stdout) == NULL))
    {
      return -1;
    }

  chdir ("/");
  printf (CLEAR);
  system ("/bin/sh");

  return 0;
}
