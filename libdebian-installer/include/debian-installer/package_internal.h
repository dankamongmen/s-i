/*
 * package_internal.h
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
 * $Id: package_internal.h,v 1.2 2003/11/03 13:46:12 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__PACKAGE_INTERNAL_H
#define DEBIAN_INSTALLER__PACKAGE_INTERNAL_H

#include <debian-installer/package.h>

typedef struct internal_di_package_parser_data internal_di_package_parser_data;

/**
 * @addtogroup di_package_parser
 * @{
 */

/**
 * @internal
 * parser info
 */
const di_parser_fieldinfo
  internal_di_package_parser_field_status,
  internal_di_package_parser_field_essential,
  internal_di_package_parser_field_priority,
  internal_di_package_parser_field_section,
  internal_di_package_parser_field_installed_size,
  internal_di_package_parser_field_maintainer,
  internal_di_package_parser_field_architecture,
  internal_di_package_parser_field_version,
  internal_di_package_parser_field_replaces,
  internal_di_package_parser_field_provides,
  internal_di_package_parser_field_depends,
  internal_di_package_parser_field_pre_depends,
  internal_di_package_parser_field_recommends,
  internal_di_package_parser_field_suggests,
  internal_di_package_parser_field_conflicts,
  internal_di_package_parser_field_enhances,
  internal_di_package_parser_field_filename,
  internal_di_package_parser_field_size,
  internal_di_package_parser_field_md5sum,
  internal_di_package_parser_field_description;

/**
 * @internal
 */
struct internal_di_package_parser_data
{
  di_packages_allocator *allocator;
  di_packages *packages;
  di_package *package;
};

/** @} */

/**
 * @addtogroup di_package
 * @{
 */

void internal_di_package_destroy_func (void *data);

/** @} */
#endif
