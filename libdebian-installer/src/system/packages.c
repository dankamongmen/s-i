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
 * $Id: packages.c,v 1.13 2004/03/09 17:10:29 waldi Exp $
 */

#include <config.h>

#include <debian-installer/system/packages.h>

#include <debian-installer/package_internal.h>
#include <debian-installer/packages_internal.h>
#include <debian-installer/parser_rfc822.h>
#include <debian-installer/slist_internal.h>

#include <ctype.h>
#include <sys/utsname.h>

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
    ),
  internal_di_system_package_parser_field_kernel_version =
    DI_PARSER_FIELDINFO (
      "Kernel-Version",
      di_parser_read_string,
      di_parser_write_string,
      offsetof (di_system_package, kernel_version)
    );

const di_parser_fieldinfo *di_system_package_parser_fieldinfo[] =
{
  &internal_di_system_package_parser_field_installer_menu_item,
  &internal_di_system_package_parser_field_subarchitecture,
  &internal_di_system_package_parser_field_kernel_version,
  NULL
};

static void internal_di_system_package_destroy_func (void *data)
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

bool di_system_package_check_subarchitecture (di_package *package, const char *subarchitecture)
{
  char *string, *begin, *end, *temp;
  int ret = false;

  if (!((di_system_package *) package)->subarchitecture)
    return true;

  string = begin = strdup (((di_system_package *) package)->subarchitecture);
  end = begin + strlen (begin);

  while (begin < end)
  {
    while (begin < end && isspace (*begin))
      begin++;
    temp = begin;
    while (temp < end && !isspace (*++temp));
    *temp = '\0';

    if (!strcmp (begin, subarchitecture))
    {
      ret = true;
      goto cleanup;
    }
    begin = temp + 1;
  }

cleanup:
  free (string);

  return false;
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

di_slist *di_system_packages_resolve_dependencies_array_permissive (di_packages *packages, di_package **array, di_packages_allocator *allocator)
{
  struct di_packages_resolve_dependencies_check s =
  {
    di_packages_resolve_dependencies_check_real,
    di_packages_resolve_dependencies_check_virtual,
    di_packages_resolve_dependencies_check_non_existant_permissive,
    { NULL, NULL },
    allocator,
    0
  };

  return di_packages_resolve_dependencies_array_special (packages, array, &s);
}

static di_package *check_virtual_kernel (di_package *package __attribute__ ((unused)), di_package *best, di_package_dependency *d)
{
  struct utsname uts;
  if (uname(&uts) == 0)
    if (((di_system_package *)d->ptr)->kernel_version &&
        strcmp (((di_system_package *)d->ptr)->kernel_version, uts.release))
      return best;
  return di_packages_resolve_dependencies_check_virtual (package, best, d);
}

void di_system_packages_resolve_dependencies_mark_kernel (di_packages *packages)
{
  struct di_packages_resolve_dependencies_check s =
  {
    di_packages_resolve_dependencies_check_real,
    check_virtual_kernel,
    di_packages_resolve_dependencies_check_non_existant,
    { NULL, NULL },
    NULL,
    0
  };

  return di_packages_resolve_dependencies_mark_special (packages, &s);
}

