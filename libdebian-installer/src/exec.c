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
 * $Id: exec.c,v 1.11 2004/01/06 15:24:52 waldi Exp $
 */

#include <config.h>

#include <debian-installer/exec.h>

#include <debian-installer/log.h>

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

int di_exec_full (const char *path, const char *const argv[], di_io_handler *stdout_handler, di_io_handler *stderr_handler, void *io_user_data, di_process_handler *parent_prepare_handler, void *parent_prepare_user_data, di_process_handler *child_prepare_handler, void *child_prepare_user_data)
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
#if 0
    int i;

    i = open ("/dev/null", O_RDONLY);
    dup2 (i, 0);
    close (i);
#endif

    if (child_prepare_handler)
      if (child_prepare_handler (pid, child_prepare_user_data))
        exit (255);

    execv (path, (char *const *) argv);
    exit (127);
  }
  else if (pid < 0)
  {
    di_log (DI_LOG_LEVEL_WARNING, "fork failed");
    return -1;
  }
  else
  {
    int i, status = -1;
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
    files[] =
    {
      { fdopen (pipeout[0], "r"), stdout_handler },
      { fdopen (pipeerr[0], "r"), stderr_handler },
    };

    fcntl (pipeout[0], F_SETFL, O_NONBLOCK);
    fcntl (pipeerr[0], F_SETFL, O_NONBLOCK);

    if (parent_prepare_handler)
      if (parent_prepare_handler (pid, parent_prepare_user_data))
      {
        kill (pid, 9);
        goto cleanup;
      }

    while (poll (fds, 2, -1) >= 0)
    {
      int exit = 0;

      for (i = 0; i < 2; i++)
      {
        if (fds[i].revents & POLLIN)
        {
          while (fgets (line, sizeof (line), files[i].file) != NULL)
            if (files[i].handler)
            {
              size_t len = strlen (line);
              if (line[len - 1] == '\n')
              {
                line[len - 1] = '\0';
                len--;
              }
              files[i].handler (line, len, io_user_data);
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

    if (!waitpid (pid, &status, 0))
      return -1;

cleanup:
    fclose (files[0].file); /* closes pipeout[0] */
    fclose (files[1].file); /* closes pipeerr[0] */

    return status;
  }

  return -1;
}

int di_exec_shell_full (const char *const cmd, di_io_handler *stdout_handler, di_io_handler *stderr_handler, void *io_user_data, di_process_handler *parent_prepare_handler, void *parent_prepare_user_data, di_process_handler *child_prepare_handler, void *child_prepare_user_data)
{
  const char *const argv[] = { "sh", "-c", cmd, NULL };
  return di_exec_full ("/bin/sh", argv, stdout_handler, stderr_handler, io_user_data, parent_prepare_handler, parent_prepare_user_data, child_prepare_handler, child_prepare_user_data); 
}

int di_exec_prepare_chdir (pid_t pid __attribute__ ((unused)), void *user_data)
{
  char *path = user_data;
  if (chdir (path))
    return -1;
  return 0;
}

int di_exec_prepare_chroot (pid_t pid __attribute__ ((unused)), void *user_data)
{
  char *path = user_data;
  if (chroot (path))
    return -1;
  if (chdir ("/"))
    return -1;
  return 0;
}

int di_exec_io_log (const char *buf, size_t len __attribute__ ((unused)), void *user_data __attribute__ ((unused)))
{
  di_log (DI_LOG_LEVEL_OUTPUT, "%s", buf);
  return 0;
}

int di_exec_mangle_status (int status)
{
  if (WIFEXITED (status))
    return WEXITSTATUS (status);
  if (WIFSIGNALED (status))
    return 128 + WTERMSIG (status);
  if (WIFSTOPPED (status))
    return 128 + WSTOPSIG (status);
  return status;
}

