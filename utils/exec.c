/*
 * Runs a program, capturing output and logging to syslog. Returns its exit
 * code.
 *
 * This is a safer replacement for shell commands like:
 *    command | logger
 * Since that loses the exit code.
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

static char ident_buf[256];

static int exec_parent (pid_t pid, void *user_data)
{
  char *program = user_data;
  snprintf (ident_buf, sizeof (ident_buf), "%s[%d]", program, pid);
  di_log_set_handler (DI_LOG_LEVEL_MASK, di_log_handler_syslog, ident_buf);
  return 0;
}

int exec_main (int argc, char **argv)
{
  int status = di_exec_full (argv[1], &argv[1], di_exec_io_log, di_exec_io_log, NULL, exec_parent, argv[1], NULL, NULL);
  return di_exec_mangle_status (status);
}

