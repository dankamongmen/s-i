/*
 * list.c
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
 * $Id: list.c,v 1.3 2003/12/11 19:29:50 waldi Exp $
 */

#include <config.h>

#include <debian-installer/list.h>

#include <debian-installer/mem.h>

static di_mem_chunk *node_mem_chunk;

/**
 * Allocate a double-linked list
 *
 * @return a di_list
 */
di_list *di_list_alloc (void)
{
  di_list *list;

  list = di_new0 (di_list, 1);
  list->first = list->last = NULL;

  return list;
}

/**
 * Append to a double-linked list
 *
 * @param list a di_list
 * @param data the data
 */
void di_list_append (di_list *list, void *data)
{
  if (!node_mem_chunk)
    node_mem_chunk = di_mem_chunk_new (sizeof (di_list_node), 4096);
  return di_list_append_chunk (list, data, node_mem_chunk);
}

/**
 * Append to a double-linked list
 *
 * @param list a di_list
 * @param data the data
 * @param mem_chunk a di_mem_chunk for allocation of new nodes
 *
 * @pre the di_mem_chunk must return chunks with at least the size of di_list_node
 */
void di_list_append_chunk (di_list *list, void *data, di_mem_chunk *mem_chunk)
{
  di_list_node *new_node;

  new_node = di_mem_chunk_alloc (mem_chunk);
  new_node->data = data;
  new_node->next = NULL;
  new_node->prev = list->last;

  if (new_node->prev)
    new_node->prev->next = new_node;
  else
    list->first = new_node;

  list->last = new_node;
}

/**
 * Prepend to a doubl-linked list
 *
 * @param list a di_list
 * @param data the data
 */
void di_list_prepend (di_list *list, void *data)
{
  if (!node_mem_chunk)
    node_mem_chunk = di_mem_chunk_new (sizeof (di_list_node), 4096);
  return di_list_prepend_chunk (list, data, node_mem_chunk);
}

/**
 * Prepend to a double-linked list
 *
 * @param list a di_list
 * @param data the data
 * @param mem_chunk a di_mem_chunk for allocation of new nodes
 *
 * @pre the di_mem_chunk must return chunks with at least the size of di_list_node
 */
void di_list_prepend_chunk (di_list *list, void *data, di_mem_chunk *mem_chunk)
{
  di_list_node *new_node;

  new_node = di_mem_chunk_alloc (mem_chunk);
  new_node->data = data;
  new_node->prev = NULL;
  new_node->next = list->first;

  if (new_node->next)
    new_node->next->prev = new_node;
  else
    list->last = new_node;

  list->first = new_node;
}

