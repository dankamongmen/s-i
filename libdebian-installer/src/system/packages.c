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
 * $Id: packages.c,v 1.4 2003/09/26 00:18:10 waldi Exp $
 */

#include <debian-installer/system/packages.h>

#include <debian-installer/package_internal.h>
#include <debian-installer/packages_internal.h>
#include <debian-installer/parser_rfc822.h>

const di_parser_fieldinfo
  internal_di_system_package_parser_field_installer_menu_item =
    DI_PARSER_FIELDINFO (
      "Installer-Menu-Item",
      di_parser_read_int,
      di_parser_write_int,
      offsetof (di_system_package, installer_menu_item)
    );

const di_parser_fieldinfo *di_system_package_parser_fieldinfo[] =
{
  &internal_di_system_package_parser_field_installer_menu_item,
  NULL
};

di_packages_allocator *di_system_packages_allocator_alloc (void)
{
  di_packages_allocator *ret;

  ret = internal_di_packages_allocator_alloc ();
  ret->package_mem_chunk = di_mem_chunk_new (sizeof (di_system_package), 16384);

  return ret;
}

di_parser_info *di_system_package_parser_info (void)
{
  di_parser_info *info;

  info = di_parser_info_alloc ();
  di_parser_info_add_pointer (info, di_package_parser_fieldinfo);
  di_parser_info_add_pointer (info, di_system_package_parser_fieldinfo);

  return info;
}

di_parser_info *di_system_packages_parser_info (void)
{
  di_parser_info *info;

  info = di_parser_info_alloc ();
  di_parser_info_add_pointer (info, di_packages_parser_fieldinfo);
  di_parser_info_add_pointer (info, di_system_package_parser_fieldinfo);

  return info;
}

di_parser_info *di_system_packages_status_parser_info (void)
{
  di_parser_info *info;

  info = di_parser_info_alloc ();
  di_parser_info_add_pointer (info, di_packages_status_parser_fieldinfo);
  di_parser_info_add_pointer (info, di_system_package_parser_fieldinfo);

  return info;
}

