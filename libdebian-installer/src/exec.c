/*
 * exec.c
 *
 * Copyright (C) 2003 Bastian Blank <waldi@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: exec.c,v 1.1 2003/08/29 12:37:33 waldi Exp $
 */

#include <debian-installer/exec.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLINE 1024

/**
 * execv like call
 *
 * @param path executable with path
 * @param argv NULL-terminated area of char pointer
 *
 * @return return code of executed process or error
 */
int di_exec (const char *path, const char *const argv[])
{
  return di_exec_full (path, argv, NULL, NULL, NULL, NULL, NULL);
}

/**
 * execv like call
 *
 * @param path executable with path
 * @param argv NULL-terminated area of char pointer
 * @param stdout_handler di_io_handler which gets stdout
 * @param stderr_handler di_io_handler which gets stderr
 * @param io_user_data user_data for di_io_handler
 * @param prepare_handler di_handler which is called before the exec
 * @param prepare_user_data user_data for di_handler
 *
 * @return return code of executed process or error
 */
int di_exec_full (const char *path, const char *const argv[], di_io_handler *stdout_handler, di_io_handler *stderr_handler, void *io_user_data, di_handler *prepare_handler, void *prepare_user_data)
{
  char line[MAXLINE];
  pid_t pid;
  int pipeout[2], pipeerr[2];

  pipe (pipeout);
  pipe (pipeerr);

  pid = fork ();

  if (pid == 0)
  {
    dup2 (pipeout[1], 1);
    dup2 (pipeerr[1], 2);
  }
  if (pid <= 0)
  {
    close (pipeout[0]);
    close (pipeerr[0]);
  }
  close (pipeout[1]);
  close (pipeerr[1]);

  if (pid == 0)
  {
    int i;

    i = open ("/dev/null", O_RDONLY);
    dup2 (i, 0);
    close (i);

    if (prepare_handler)
      prepare_handler (prepare_user_data);

    execv (path, (char *const *) argv);
    exit (127);
  }
  else if (pid < 0)
  {
  }
  else
  {
    int i, n, status;
    struct pollfd fds[2] =
    {
      { pipeout[0], POLLIN, 0 },
      { pipeerr[0], POLLIN, 0 },
    };
    struct files
    {
      FILE *file;
      di_io_handler *handler;
    }
    files[2];

    fcntl (pipeout[0], F_SETFL, O_NONBLOCK);
    fcntl (pipeerr[0], F_SETFL, O_NONBLOCK);
    files[0].file = fdopen (pipeout[0], "r");
    files[1].file = fdopen (pipeerr[0], "r");
    files[0].handler = stdout_handler;
    files[1].handler = stderr_handler;

    while (poll (fds, 2, -1) >= 0)
    {
      int exit = 0;

      for (i = 0; i < 2; i++)
      {
        if (fds[i].revents & POLLIN)
        {
          while (fgets (line, MAXLINE, files[i].file) != NULL)
          {
            n = strlen (line);
            line[n - 1] = '\0';
            if (files[i].handler)
              files[i].handler (line, n, io_user_data);
          }
          exit = 1;
        }
      }

      if (exit)
        continue;

      for (i = 0; i < 2; i++)
        if (fds[i].revents & POLLHUP)
          exit = 1;

      if (exit)
        break;
    }

    fclose (files[0].file); /* closes pipeout[0] */
    fclose (files[1].file); /* closes pipeerr[0] */

    if (!waitpid (pid, &status, 0))
      return -1;

    return WIFEXITED(status) ? WEXITSTATUS(status) : status;
  }

  return -1;
}

/**
 * system like call
 *
 * @param cmd command
 *
 * @return return code of executed process or error
 */
int di_exec_shell (const char *const cmd)
{
  return di_exec_shell_full (cmd, NULL, NULL, NULL, NULL, NULL);
}

/**
 * system like call
 *
 * @param cmd command
 * @param stdout_handler di_io_handler which gets stdout
 * @param stderr_handler di_io_handler which gets stderr
 * @param io_user_data user_data for di_io_handler
 * @param prepare_handler di_handler which is called before the exec
 * @param prepare_user_data user_data for di_handler
 *
 * @return return code of executed process or error
 */
int di_exec_shell_full (const char *const cmd, di_io_handler *stdout_handler, di_io_handler *stderr_handler, void *io_user_data, di_handler *prepare_handler, void *prepare_user_data)
{
  const char *const argv[] = { "sh", "-c", cmd, NULL };
  return di_exec_full ("/bin/sh", argv, stdout_handler, stderr_handler, io_user_data, prepare_handler, prepare_user_data); 
}

/**
 * system like call which output via di_log
 *
 * @param cmd command
 *
 * @return return code of executed process or error
 */
int di_exec_shell_log (const char *const cmd)
{
  return di_exec_shell_full (cmd, di_exec_io_log, di_exec_io_log, NULL, NULL, NULL);
}

/**
 * chdir to user_data
 *
 * @param user_data path
 */
int di_exec_prepare_chroot (void *user_data)
{
  char *path = user_data;
  if (chroot (path))
    return -1;
  if (chdir ("/"))
    return -1;
  return 0;
}

/**
 * logs to di_log
 */
int di_exec_io_log (const char *buf, size_t n __attribute__ ((unused)), void *user_data __attribute__ ((unused)))
{
  di_log (DI_LOG_LEVEL_OUTPUT, buf);
  return 0;
}

