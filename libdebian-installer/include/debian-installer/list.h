/*
 * list.h
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
 * $Id: list.h,v 1.2 2003/11/19 09:24:14 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__LIST_H
#define DEBIAN_INSTALLER__LIST_H

#include <debian-installer/mem_chunk.h>

typedef struct di_list di_list;
typedef struct di_list_node di_list_node;

/**
 * @addtogroup di_list
 * @{
 */

/**
 * @brief Double-linked list
 */
struct di_list
{
  di_list_node *first;                                  /**< first node */
  di_list_node *last;                                   /**< last node */
};

/**
 * @brief Node of a double-linked list
 */
struct di_list_node
{
  di_list_node *next;                                   /**< next node */
  di_list_node *prev;                                   /**< previsous node */
  void *data;                                           /**< data */
};

di_list *di_list_alloc (void);
void di_list_append (di_list *list, void *data) __attribute__ ((nonnull(1)));
void di_list_append_chunk (di_list *list, void *data, di_mem_chunk *mem_chunk) __attribute__ ((nonnull(1,3)));
void di_list_prepend (di_list *list, void *data) __attribute__ ((nonnull(1)));
void di_list_prepend_chunk (di_list *list, void *data, di_mem_chunk *mem_chunk) __attribute__ ((nonnull(1,3)));

/** @} */
#endif
