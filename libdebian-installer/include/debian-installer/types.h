/*
 * types.h
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *               2003 Bastian Blank <waldi@debian.org>
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
 * $Id: types.h,v 1.1 2003/08/29 12:37:33 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__TYPES_H
#define DEBIAN_INSTALLER__TYPES_H

#include <debian-installer/log.h>

#include <stdint.h>
#include <sys/types.h>

/**
 * @defgroup di_types Types definitions
 * @{
 */
typedef int di_equal_func (const void *key1, const void *key2);
typedef void di_destroy_notify (void *data);
typedef uint32_t di_hash_func (const void *key);
typedef void di_hfunc (void *key, void *value, void *user_data);
typedef void di_func (void *data, void *user_data);

typedef int di_handler (void *user_data);
typedef int di_io_handler (const char *buf, size_t n, void *user_data);

/**
 * small size type used in many internal structures
 */
typedef uint32_t di_ksize_t;

/** @} */
#endif
