#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <debian-installer.h>
#include <cdebconf/debconfclient.h>

char *new_argv[] = 
{
  "sh",
  NULL
};

char *new_envp[] = 
{
  "PATH=",
  "TERM=",
  NULL
};

int main (int argc __attribute__ ((unused)), char *argv[] __attribute__ ((unused)), char *envp[]) {
  struct debconfclient *client;
  char **t1, **t2;
  pid_t pid;
  int i;

  client = debconfclient_new();

  debconf_capb(client, "backup");
  debconf_fset(client, "di-utils-shell/do-shell", "seen", "false");
  debconf_input(client, "high", "di-utils-shell/do-shell");
  if (debconf_go(client) == 30)
    exit(10);

  /* 
   * To run the shell, we must first close the cdebconf file
   * descriptors. That's why this postinst is a C program in
   * the first place.
   */
  for (i = 0; i <= 2; i++)
    if (dup2(DEBCONF_OLD_FD_BASE + i, i) == -1)
      exit(1);

  t1 = new_envp;
  while (*t1)
  {
    t2 = envp;
    while (*t2)
    {
      if (strncmp (*t1, *t2, strlen (*t1)) == 0)
      {
        *t1 = *t2;
        break;
      }
      t2++;
    }
    t1++;
  }

  pid = fork ();

  if (pid == 0)
  {
    chdir("/");

    execve ("/bin/sh", new_argv, new_envp);
  }
  else if (pid > 0)
  {
    int status;
    if (!waitpid (pid, &status, 0))
      return 0;
  }

  return 0;
}

