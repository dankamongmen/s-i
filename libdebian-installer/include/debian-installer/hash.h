/*
 * hash.h
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
 * $Id: hash.h,v 1.3 2003/09/29 12:10:00 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__HASH_H
#define DEBIAN_INSTALLER__HASH_H

#include <debian-installer/mem.h>
#include <debian-installer/types.h>

/** 
 * @defgroup di_hash Hash functions
 * @{
 */

di_equal_func di_rstring_equal;
di_hash_func di_rstring_hash;

#if 0
di_equal_func di_string_equal;
di_hash_func di_string_hash;
#endif

/**
 * @defgroup di_hash_table Simple hash table
 */
/** @} */

typedef struct di_hash_table di_hash_table;

/**
 * @addtogroup di_hash_table
 * @{
 */

di_hash_table *di_hash_table_new (di_hash_func hash_func, di_equal_func key_equal_func);
di_hash_table *di_hash_table_new_full (di_hash_func hash_func, di_equal_func key_equal_func, di_destroy_notify key_destroy_func, di_destroy_notify value_destroy_func);
void di_hash_table_destroy (di_hash_table *hash_table);
void di_hash_table_insert (di_hash_table *hash_table, void *key, void *value);
void *di_hash_table_lookup (di_hash_table *hash_table, const void *key);
void di_hash_table_foreach (di_hash_table *hash_table, di_hfunc *func, void *user_data);
di_ksize_t di_hash_table_size (di_hash_table *hash_table);

/** @} */
#endif
