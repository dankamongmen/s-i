/*
 * packages.c
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
 * $Id: packages.c,v 1.2 2003/09/15 20:02:47 waldi Exp $
 */

#include <debian-installer/system/packages.h>

#include <debian-installer/packages_internal.h>
#include <debian-installer/parser_rfc822.h>

const di_parser_fieldinfo
  internal_di_system_packages_parser_field_installer_menu_item =
    DI_PARSER_FIELDINFO (
      "Installer-Menu-Item",
      di_parser_read_int,
      di_parser_write_int,
      offsetof (di_system_package, installer_menu_item)
    );

const di_parser_fieldinfo *di_system_packages_parser_fieldinfo[] =
{
  &internal_di_system_packages_parser_field_installer_menu_item,
  NULL
};

di_packages_allocator *di_system_packages_allocator_alloc (void)
{
  di_packages_allocator *ret;

  ret = internal_di_packages_allocator_alloc ();
  ret->package_mem_chunk = di_mem_chunk_new (sizeof (di_system_package), 16384);

  return ret;
}

void di_system_packages_allocator_free (di_packages_allocator *allocator)
{
  di_packages_allocator_free (allocator);
}

di_parser_info *internal_di_system_packages_parser_info (void)
{
  di_parser_info *info;

  info = di_parser_info_alloc ();
  di_parser_info_add_pointer (info, di_packages_parser_fieldinfo);
  di_parser_info_add_pointer (info, di_system_packages_parser_fieldinfo);

  return info;
}

di_packages *di_system_packages_read_file (const char *file)
{
  di_packages *packages;
  di_parser_info *info;

  packages = di_system_packages_alloc ();
  info = internal_di_system_packages_parser_info ();

  if (di_parser_rfc822_read_file (file, info, NULL, NULL, packages) < 0)
  {
    di_packages_free (packages);
    return NULL;
  }

  di_parser_info_free (info);

  return packages;
}

int di_system_packages_write_file (di_packages *packages, const char *file)
{
  di_parser_info *info;
  int ret;

  info = internal_di_system_packages_parser_info ();
  ret = di_parser_rfc822_write_file (file, info, internal_di_packages_parser_write_entry_next, packages);
  di_parser_info_free (info);

  return ret;
}

