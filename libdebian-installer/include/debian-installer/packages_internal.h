/*
 * packages_internal.h
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
 * $Id: packages_internal.h,v 1.5 2004/03/09 17:10:28 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__PACKAGES_INTERNAL_H
#define DEBIAN_INSTALLER__PACKAGES_INTERNAL_H

#include <debian-installer/packages.h>

typedef struct di_packages_resolve_dependencies_check di_packages_resolve_dependencies_check;

/**
 * @addtogroup di_packages
 * @{
 */

di_packages_allocator *internal_di_packages_allocator_alloc (void);

typedef bool di_packages_resolve_dependencies_check_package (di_packages_resolve_dependencies_check *r, di_package *package, di_package_dependency *d);
typedef di_package *di_packages_resolve_dependencies_check_provide (di_package *package, di_package *best, di_package_dependency *d);

struct di_packages_resolve_dependencies_check
{
  di_packages_resolve_dependencies_check_package *check_real;
  di_packages_resolve_dependencies_check_provide *check_virtual;
  di_packages_resolve_dependencies_check_package *check_non_existant;
  di_slist install;
  di_packages_allocator *allocator;
  unsigned int resolver;
};

di_slist *di_packages_resolve_dependencies_special (di_packages *packages, di_slist *list, di_packages_resolve_dependencies_check *s);
di_slist *di_packages_resolve_dependencies_array_special (di_packages *packages, di_package **array, di_packages_resolve_dependencies_check *s);
void di_packages_resolve_dependencies_mark_special (di_packages *packages, di_packages_resolve_dependencies_check *s);

void di_packages_resolve_dependencies_marker (di_packages *packages);
bool di_packages_resolve_dependencies_recurse (di_packages_resolve_dependencies_check *r, di_package *package, di_package *dependend_package);

di_packages_resolve_dependencies_check_package
  di_packages_resolve_dependencies_check_real,
  di_packages_resolve_dependencies_check_real_dependency,
  di_packages_resolve_dependencies_check_non_existant,
  di_packages_resolve_dependencies_check_non_existant_quiet,
  di_packages_resolve_dependencies_check_non_existant_permissive;

di_packages_resolve_dependencies_check_provide
  di_packages_resolve_dependencies_check_virtual;

/** @} */

/**
 * @addtogroup di_packages_parser
 * @{
 */

di_parser_write_entry_next
  internal_di_packages_parser_write_entry_next;

const di_parser_fieldinfo
  internal_di_packages_parser_field_package;

/** @} */
#endif
