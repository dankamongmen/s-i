/*
 * packages.h
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
 * $Id: packages.h,v 1.1 2003/08/29 12:37:33 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__SYSTEM__PACKAGES_H
#define DEBIAN_INSTALLER__SYSTEM__PACKAGES_H

#include <debian-installer/packages.h>

typedef struct di_system_package di_system_package;

/**
 * @defgroup di_system_packages Packages file - system
 * @{
 */

/**
 * @brief Package
 */
struct di_system_package
{
  di_package p;
  int installer_menu_item;
};

di_packages *di_system_packages_alloc (void);
void di_system_packages_free (di_packages *packages);

di_packages *di_system_packages_read_file (const char *file);
int di_system_packages_write_file (di_packages *packages, const char *file);

extern const di_parser_fieldinfo *di_system_packages_parser_fieldinfo[];

/** @} */
#endif
