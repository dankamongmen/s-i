/*
 * slist.h
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
 * $Id: slist.h,v 1.1 2003/08/29 12:37:33 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__SLIST_H
#define DEBIAN_INSTALLER__SLIST_H

#include <debian-installer/mem.h>
#include <debian-installer/types.h>

typedef struct di_slist di_slist;
typedef struct di_slist_node di_slist_node;

/**
 * @defgroup di_slist Single-linked list
 * @{
 */
 
/**
 * @brief Single-linked list
 */
struct di_slist
{
  di_slist_node *first;                                 /**< first node */
  di_slist_node *last;                                  /**< last node */
};

/**
 * @brief Node of a single-linked list
 */
struct di_slist_node
{
  di_slist_node *next;                                  /**< next node */
  void *data;                                           /**< data */
};

di_slist *di_slist_alloc (void);
void di_slist_free (di_slist *slist);
void di_slist_append (di_slist *slist, void *data) __attribute__ ((nonnull(1)));
void di_slist_append_chunk (di_slist *slist, void *data, di_mem_chunk *mem_chunk) __attribute__ ((nonnull(1,3)));
void di_slist_prepend (di_slist *slist, void *data) __attribute__ ((nonnull(1)));
void di_slist_prepend_chunk (di_slist *slist, void *data, di_mem_chunk *mem_chunk) __attribute__ ((nonnull(1,3)));
void di_slist_foreach (di_slist *slist, di_func *func, void *user_data);

/** @} */
#endif
