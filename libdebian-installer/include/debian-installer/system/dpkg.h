/*
 * dpkg.h
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
 * $Id: dpkg.h,v 1.2 2003/09/15 20:02:46 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__SYSTEM__DPKG_H
#define DEBIAN_INSTALLER__SYSTEM__DPKG_H

#include <debian-installer/packages.h>

#include <stdbool.h>

int di_system_dpkg_package_configure (di_packages *status, const char *_package, bool force);
di_package *di_system_dpkg_package_control_read (di_packages_allocator *allocator, const char *filename);
int di_system_dpkg_package_control_file_exec (di_packages *status, const char *_package, const char *name, int argc, const char *const argv[]);
int di_system_dpkg_package_unpack (di_packages *status, const char *package, const char *filename);

#endif
