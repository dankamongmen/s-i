/*
 * log.c
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *               2003 Bastian Blank <waldi@debian.org>
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
 * $Id: log.c,v 1.1 2003/08/29 12:37:33 waldi Exp $
 */

#include <debian-installer/log.h>

#include <debian-installer/slist.h>
#include <debian-installer/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct di_log_handler_struct di_log_handler_struct;

/**
 * @addtogroup di_log
 */

/**
 * @internal
 * @brief Log handler info
 */
struct di_log_handler_struct
{
  unsigned int id;                                      /**< unique id */
  di_log_level_flags log_level;                         /**< flags */
  di_log_handler *log_func;                             /**< the handler */
  void *user_data;                                      /**< supplied data */
};

static di_slist handlers;

#define ALERT_LEVELS (DI_LOG_LEVEL_ERROR | DI_LOG_LEVEL_CRITICAL | DI_LOG_LEVEL_WARNING)

void mklevel_prefix (char *level_prefix, di_log_level_flags log_level)
{
  switch (log_level & DI_LOG_LEVEL_MASK)
  {
    case DI_LOG_LEVEL_ERROR:
      strcat (level_prefix, "ERROR");
      break;
    case DI_LOG_LEVEL_CRITICAL:
      strcat (level_prefix, "CRITICAL");
      break;
    case DI_LOG_LEVEL_WARNING:
      strcat (level_prefix, "WARNING");
      break;
    case DI_LOG_LEVEL_MESSAGE:
      strcat (level_prefix, "Message");
      break;
    case DI_LOG_LEVEL_INFO:
      strcat (level_prefix, "INFO");
      break;
    case DI_LOG_LEVEL_DEBUG:
      strcat (level_prefix, "DEBUG");
      break;
    case DI_LOG_LEVEL_OUTPUT:
      return;
    default:
      if (log_level)
      {
        char buf[16];
        strcat (level_prefix, "LOG-");
        sprintf (buf, "%u", log_level & DI_LOG_LEVEL_MASK);
        strcat (level_prefix, buf);
      }
      else
        strcat (level_prefix, "LOG");
      break;
  }

  if (log_level & ALERT_LEVELS)
    strcat (level_prefix, " **");
  strcat (level_prefix, ": ");
}

static void di_log_handler_default (di_log_level_flags log_level, const char *message, void *user_data __attribute__ ((unused)))
{
  char buf[1280] = { '\0' };
  const char *progname = di_progname_get ();

  if (!progname)
    sprintf (buf, "(process:%lu): ", (unsigned long) getpid ());
  else
    sprintf (buf, "(%s:%lu): ", progname, (unsigned long) getpid ());

  mklevel_prefix (buf, log_level);

  if (message)
    strcat (buf, message);
  else
    strcat (buf, "(NULL)");

  strcat (buf, "\n");

  if (log_level & ALERT_LEVELS)
    fputs (buf, stderr);
  else
    fputs (buf, stdout);
}

void di_log_handler_syslog (di_log_level_flags log_level, const char *message, void *user_data __attribute__ ((unused)))
{
  char buf[1280] = { '\0' };
  static char ident_buf[129];
  static int log_open;
  int syslog_level;

  if (!log_open)
  {
    if (user_data)
      strncpy (ident_buf, user_data, 128);
    else
    {
      const char *progname = di_progname_get ();
      if (!progname)
        snprintf (ident_buf, 128, "process[%lu]", (unsigned long) getpid ());
      else
        snprintf (ident_buf, 128, "%s[%lu]", progname, (unsigned long) getpid ());
    }

    openlog (ident_buf, 0, LOG_USER);
    log_open = 1;
  }

  mklevel_prefix (buf, log_level);
  strcat (buf, ": ");

  if (message)
    strcat (buf, message);
  else
    strcat (buf, "(NULL)");

  strcat (buf, "\n");

  switch (log_level)
  {
    case DI_LOG_LEVEL_ERROR:
      syslog_level = LOG_ALERT;
      break;
    case DI_LOG_LEVEL_CRITICAL:
      syslog_level = LOG_CRIT;
      break;
    case DI_LOG_LEVEL_WARNING:
      syslog_level = LOG_WARNING;
      break;
    case DI_LOG_LEVEL_MESSAGE:
      syslog_level = LOG_NOTICE;
      break;
    case DI_LOG_LEVEL_INFO:
      syslog_level = LOG_INFO;
      break;
    case DI_LOG_LEVEL_DEBUG:
    default:
      syslog_level = LOG_DEBUG;
      break;
  }

  syslog (syslog_level, buf);
}

static di_log_handler *di_log_get_handler (di_log_level_flags log_level, void **user_data)
{
  di_slist_node *node;

  for (node = handlers.first; node; node = node->next)
  {
    di_log_handler_struct *handler = node->data;

    if ((handler->log_level & log_level) == log_level)
    {
      *user_data = handler->user_data;
      return handler->log_func;
    }
  }

  return di_log_handler_default;
}

unsigned int di_log_set_handler (di_log_level_flags log_levels, di_log_handler *log_func, void *user_data)
{
  static unsigned int handler_id = 0;
  di_log_handler_struct *handler;

  handler = di_new (di_log_handler_struct, 1);

  handler->id = ++handler_id;
  handler->log_level = log_levels;
  handler->log_func = log_func;
  handler->user_data = user_data;

  di_slist_append (&handlers, handler);

  return handler_id;
}

void di_log (di_log_level_flags log_level, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  di_logv (log_level, format, args);
  va_end (args);
}

void di_logv (di_log_level_flags log_level, const char *format, va_list args)
{
  char buf[1025];
  int fatal = log_level & DI_LOG_FATAL_MASK;
  di_log_handler *log_func;
  void *user_data;

  vsnprintf (buf, 1024, format, args);

  log_func = di_log_get_handler (log_level, &user_data);

  log_func (log_level, buf, user_data);

  if (fatal)
    exit (1);
}

