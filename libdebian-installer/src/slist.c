/*
 * slist.c
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
 * $Id: slist.c,v 1.3 2003/09/30 19:22:07 waldi Exp $
 */

#include <debian-installer/slist.h>

static di_mem_chunk *node_mem_chunk;

/**
 * Allocate a single-linked list
 *
 * @return a di_slist
 */
di_slist *di_slist_alloc (void)
{
  di_slist *slist;

  slist = di_new0 (di_slist, 1);

  return slist;
}

/**
 * Free a single-linked list
 *
 * @param slist a di_slist
 */
void di_slist_free (di_slist *slist)
{
  di_free (slist);
}

/**
 * Append to a single-linked list
 *
 * @param slist a di_slist
 * @param data the data
 */
void di_slist_append (di_slist *slist, void *data)
{
  if (!node_mem_chunk)
    node_mem_chunk = di_mem_chunk_new (sizeof (di_slist_node), 4096);
  return di_slist_append_chunk (slist, data, node_mem_chunk);
}

/**
 * Append to a single-linked list
 *
 * @param slist a di_slist
 * @param data the data
 * @param mem_chunk a di_mem_chunk for allocation of new nodes
 *
 * @pre the di_mem_chunk must return chunks with at least the size of di_slist_node
 */
void di_slist_append_chunk (di_slist *slist, void *data, di_mem_chunk *mem_chunk)
{
  di_slist_node *new_node;

  new_node = di_mem_chunk_alloc (mem_chunk);
  new_node->data = data;
  new_node->next = NULL;

  if (slist->bottom)
    slist->bottom->next = new_node;
  else
    slist->head = new_node;

  slist->bottom = new_node;
}

/**
 * Prepend to a single-linked list
 *
 * @param slist a di_slist
 * @param data the data
 */
void di_slist_prepend (di_slist *slist, void *data)
{
  if (!node_mem_chunk)
    node_mem_chunk = di_mem_chunk_new (sizeof (di_slist_node), 4096);
  return di_slist_prepend_chunk (slist, data, node_mem_chunk);
}

/**
 * Prepend to a single-linked list
 *
 * @param slist a di_slist
 * @param data the data
 * @param mem_chunk a di_mem_chunk for allocation of new nodes
 *
 * @pre the di_mem_chunk must return chunks with at least the size of di_slist_node
 */
void di_slist_prepend_chunk (di_slist *slist, void *data, di_mem_chunk *mem_chunk)
{
  di_slist_node *new_node;

  new_node = di_mem_chunk_alloc (mem_chunk);
  new_node->data = data;
  new_node->next = slist->head;

  if (!new_node->next)
    slist->bottom = new_node;

  slist->head = new_node;
}

/**
 * Iterate over a single-linked list
 *
 * @param slist a di_slist
 * @param func this is called with every list item
 * @param user_data user_data
 */
#if 0
void di_slist_foreach (di_slist *slist, di_func *func, void *user_data)
{
  di_slist_node *node;

  for (node = slist->head; node; node = node->next)
    func (node->data, user_data);
}
#endif
