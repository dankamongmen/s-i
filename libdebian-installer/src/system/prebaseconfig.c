/*
 * prebaseconfig.c
 *
 * Copyright (C) 2000-2002 David Kimdon <dwhedon@debian.org>
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
 * $Id: prebaseconfig.c,v 1.1 2003/08/29 12:37:33 waldi Exp $
 */

#include <debian-installer/system/prebaseconfig.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

int di_prebaseconfig_append (const char *udeb, const char *fmt, ...)
{
  char path[129];
  FILE *fp;
  va_list ap;

  if (snprintf (path, 128, DI_PREBASECONFIG_DIR "/%s", udeb) == -1) 
    return -1;

  if ((fp = fopen (path, "a")) == NULL) 
    return -1;

  fputs ("\n# start entry\n", fp);

  va_start(ap, fmt);
  vfprintf(fp, fmt, ap);
  va_end(ap);

  fputs ("\n# end entry\n", fp);

  fclose (fp);

  return 0;
}

