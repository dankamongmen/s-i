/*
 * udpkg.c
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
 * $Id: udpkg.c,v 1.1 2003/10/01 10:43:17 waldi Exp $
 */

#include <debian-installer.h>

#include <getopt.h>
#include <stdbool.h>

di_packages *status;
di_packages_allocator *status_allocator;
bool status_changed = false;

int force_configure = false;

int status_read (void)
{
  status_allocator = di_system_packages_allocator_alloc ();
  status = di_system_packages_status_read_file (DI_SYSTEM_DPKG_STATUSFILE, status_allocator);

  return !status;
}

void status_write (void)
{
  if (status_changed)
    di_system_packages_status_write_file (status, DI_SYSTEM_DPKG_STATUSFILE);
}

int udpkg_configure (int argc, char **argv)
{
}

int udpkg_install (int argc, char **argv)
{
}

int udpkg_unpack (int argc, char **argv)
{
}

enum parts
{
  UDPKG_CONFIGURE,
  UDPKG_INSTALL,
  UDPKG_UNPACK,
};

int udpkg_main (int argc, char **argv)
{
  int c;
  enum parts part = -1;
  struct option longopts[] =
  {
    { "configure", 0, 0, 'c' },
    { "install", 0, 0, 'i' },
    { "unpack", 0, 0, 'u' },
    { "force-configure", 0, &force_configure, true },
    { 0, 0, 0, 0 }
  };

  while ((c = getopt_long (argc, argv, "ciu", longopts, NULL)) != -1)
  {
    switch (c)
    {
      case 'c':
      case 'i':
      case 'u':
        if ((int) part != -1)
          return 1;
        switch (c)
        {
          case 'c':
            part = UDPKG_CONFIGURE;
            break;
          case 'i':
            part = UDPKG_INSTALL;
            break;
          case 'u':
            part = UDPKG_UNPACK;
            break;
        }
    }

  }

  if (status_read ())
    return 1;
  
  switch (part)
  {
    case UDPKG_CONFIGURE:
      udpkg_configure (argc, argv);
    case UDPKG_INSTALL:
      udpkg_install (argc, argv);
    case UDPKG_UNPACK:
      udpkg_unpack (argc, argv);
  }

  status_write ();

  return 0;
}

