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
 * $Id: packages.c,v 1.7 2003/10/03 13:57:12 waldi Exp $
 */

#include <debian-installer/system/packages.h>

#include <debian-installer/package_internal.h>
#include <debian-installer/packages_internal.h>
#include <debian-installer/parser_rfc822.h>

#include <ctype.h>

const di_parser_fieldinfo
  internal_di_system_package_parser_field_installer_menu_item =
    DI_PARSER_FIELDINFO (
      "Installer-Menu-Item",
      di_parser_read_int,
      di_parser_write_int,
      offsetof (di_system_package, installer_menu_item)
    ),
  internal_di_system_package_parser_field_subarchitecture =
    DI_PARSER_FIELDINFO (
      "Subarchitecture",
      di_parser_read_string,
      di_parser_write_string,
      offsetof (di_system_package, subarchitecture)
    );

const di_parser_fieldinfo *di_system_package_parser_fieldinfo[] =
{
  &internal_di_system_package_parser_field_installer_menu_item,
  &internal_di_system_package_parser_field_subarchitecture,
  NULL
};

void internal_di_system_package_destroy_func (void *data)
{
  di_system_package_destroy (data);
}

void di_system_package_destroy (di_system_package *package)
{
  di_free (package->subarchitecture);

  di_package_destroy (&package->p);
}

di_packages_allocator *di_system_packages_allocator_alloc (void)
{
  di_packages_allocator *ret;

  ret = internal_di_packages_allocator_alloc ();
  ret->package_mem_chunk = di_mem_chunk_new (sizeof (di_system_package), 16384);

  return ret;
}

di_packages *di_system_packages_alloc (void)
{
  di_packages *ret;

  ret = di_new0 (di_packages, 1);
  ret->table = di_hash_table_new_full (di_rstring_hash, di_rstring_equal, NULL, internal_di_system_package_destroy_func);

  return ret;
}

int di_system_package_check_subarchitecture (di_package *package, const char *subarchitecture)
{
  char *string, *begin, *end, *temp;
  int ret = 1;

  string = begin = strdup (package->package);
  end = begin + strlen (begin);

  while (begin < end)
  {
    while (begin < end && isspace (*begin))
      begin++;
    temp = begin;
    while (temp < end && !isspace (*++temp));
    *temp = '\0';

    printf ("get: %s\n", begin);
    if (!strcmp (begin, subarchitecture))
    {
      printf ("found it\n");
      ret = 1;
      goto cleanup;
    }
    begin = temp + 1;
  }

cleanup:
  free (string);

  return ret;
}

di_parser_info *di_system_package_parser_info (void)
{
  di_parser_info *info;

  info = di_package_parser_info ();
  di_parser_info_add (info, di_system_package_parser_fieldinfo);

  return info;
}

di_parser_info *di_system_packages_parser_info (void)
{
  di_parser_info *info;

  info = di_packages_parser_info ();
  di_parser_info_add (info, di_system_package_parser_fieldinfo);

  return info;
}

di_parser_info *di_system_packages_status_parser_info (void)
{
  di_parser_info *info;

  info = di_packages_status_parser_info ();
  di_parser_info_add (info, di_system_package_parser_fieldinfo);

  return info;
}

