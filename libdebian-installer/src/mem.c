/*
 * mem.c
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
 * $Id: mem.c,v 1.5 2003/11/13 21:28:57 waldi Exp $
 */

#include <debian-installer/mem.h>

#include <debian-installer/macros.h>
#include <debian-installer/log.h>

#include <stdlib.h>
#include <string.h>

/**
 * Allocate memory
 *
 * @param n_bytes size in bytes
 *
 * @post never returns NULL
 */
void *di_malloc (size_t n_bytes)
{
  void *mem;

  mem = malloc (n_bytes);

  if (!mem)
    di_error ("%s: failed to allocate %zu bytes", DI_STRLOC, n_bytes);
  return mem;
}

/**
 * Allocate cleared memory
 *
 * @param n_bytes size in bytes
 *
 * @post never returns NULL
 */
void *di_malloc0 (size_t n_bytes)
{
  void *mem;

  mem = calloc (1, n_bytes);

  if (!mem)
    di_error ("%s: failed to allocate %zu bytes", DI_STRLOC, n_bytes);
  return mem;
}

/**
 * Reallocate memory
 *
 * @param mem memory
 * @param n_bytes size in bytes
 *
 * @post never returns NULL
 */
void *di_realloc (void *mem, size_t n_bytes)
{
  mem = realloc (mem, n_bytes);

  if (!mem)
    di_error ("%s: failed to allocate %zu bytes", DI_STRLOC, n_bytes);
  return mem;
}

/**
 * Free memory
 *
 * @param mem memory
 */
void di_free (void *mem)
{
  free (mem);
}

