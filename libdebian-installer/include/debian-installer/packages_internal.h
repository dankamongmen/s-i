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
 * $Id: packages_internal.h,v 1.4 2003/11/20 20:02:48 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__PACKAGES_INTERNAL_H
#define DEBIAN_INSTALLER__PACKAGES_INTERNAL_H

#include <debian-installer/packages.h>

/**
 * @addtogroup di_packages
 * @{
 */

di_packages_allocator *internal_di_packages_allocator_alloc (void);

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
