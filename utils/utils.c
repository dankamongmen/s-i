/*
 * utils.c
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
 * $Id: utils.c,v 1.2 2003/10/16 23:01:20 joeyh Rel $
 */

#include <debian-installer.h>

extern int mapdevfs_main (int argv, char **argc);

static struct applet
{
  const char *name;
  int (*main) (int argv, char **argc);
}
applets[] =
{
  { "mapdevfs", mapdevfs_main },
};

static int applet_name_compare (const void *x, const void *y)
{
  const char *name = x;
  const struct applet *applet = y;

  return strcmp (name, applet->name);
}

static int call_applet (int argc, char **argv, int recurse)
{
  const char *applet_name, *s;
  struct applet *applet;

  if (argc <= 0)
    return 1;

  applet_name = argv[0];

  for (s = applet_name; *s;)
    if (*s++ == '/')
      applet_name = s;
  
  applet = bsearch (applet_name, applets, sizeof (applets) / sizeof (struct applet), sizeof (struct applet), applet_name_compare);
  
  if (applet)
    return applet->main (argc, argv);

  if (!recurse)
  {
    argc--;
    argv++;
    return call_applet (argc, argv, 1);
  }

  fprintf(stderr, "Usage: di-utils [command] [args]\n");
  return 1;
}

int main (int argc, char **argv)
{
  return call_applet (argc, argv, 0);
}
