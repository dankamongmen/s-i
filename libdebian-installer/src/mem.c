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
 * $Id: mem.c,v 1.2 2003/09/29 12:10:00 waldi Exp $
 */

#include <debian-installer/mem.h>

#include <debian-installer/macros.h>
#include <debian-installer/log.h>

#include <stdlib.h>
#include <string.h>

#define MEM_ALIGN sizeof (void *)
#define MEM_AREA_SIZE 4

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

typedef struct di_mem_area di_mem_area;

/**
 * @addtogroup di_mem_chunk
 * @{
 */

/**
 * @internal
 * @brief a mem chunk
 */
struct di_mem_chunk
{
  int num_mem_areas;            /**< the number of memory areas */
  int num_marked_areas;         /**< the number of areas marked for deletion */
  di_ksize_t atom_size;         /**< the size of an atom */
  di_ksize_t area_size;         /**< the size of a memory area */
  di_ksize_t rarea_size;        /**< the size of a real memory area */
  di_mem_area *mem_area;        /**< the current memory area */
  di_mem_area *mem_areas;       /**< a list of all the mem areas owned by this chunk */
};

/**
 * @internal
 * @brief a mem area
 */
struct di_mem_area
{
  di_mem_area *next;           /**< the next mem area */
  di_mem_area *prev;           /**< the previous mem area */
  di_ksize_t index;            /**< the current index into the "mem" array */
  di_ksize_t free;             /**< the number of free bytes in this mem area */
  di_ksize_t allocated;        /**< the number of atoms allocated from this area */
  char mem[MEM_AREA_SIZE];     /**< the mem array from which atoms get allocated
                                *   the actual size of this array is determined by
                                *   the mem chunk "area_size". ANSI says that it
                                *   must be declared to be the maximum size it
                                *   can possibly be (even though the actual size
                                *   may be less).
                                */
};

static size_t di_mem_chunk_compute_size (di_ksize_t size, di_ksize_t min_size) __attribute__ ((nonnull));

/** @} */

/**
 * Makes a new Memory-Chunk Allocer
 *
 * @param atom_size size of each piece
 * @param area_size size of each alloced chunk
 */
di_mem_chunk* di_mem_chunk_new (di_ksize_t atom_size, di_ksize_t area_size)
{
  di_mem_chunk *mem_chunk;

  if (area_size < atom_size)
    return NULL;

  area_size = (area_size + atom_size - 1) / atom_size;
  area_size *= atom_size;

  mem_chunk = di_new (di_mem_chunk, 1);
  mem_chunk->num_mem_areas = 0;
  mem_chunk->num_marked_areas = 0;
  mem_chunk->mem_area = NULL;
  mem_chunk->mem_areas = NULL;
  mem_chunk->atom_size = atom_size;

  if (mem_chunk->atom_size % MEM_ALIGN)
    mem_chunk->atom_size += MEM_ALIGN - (mem_chunk->atom_size % MEM_ALIGN);

  mem_chunk->rarea_size = di_mem_chunk_compute_size (area_size + sizeof (di_mem_area) - MEM_AREA_SIZE, atom_size + sizeof (di_mem_area) - MEM_AREA_SIZE);
  mem_chunk->area_size = mem_chunk->rarea_size - (sizeof (di_mem_area) - MEM_AREA_SIZE);

  return mem_chunk;
}

/**
 * Allocate a piece
 *
 * @param mem_chunk a di_mem_chunk
 *
 * @return memory
 */
void *di_mem_chunk_alloc (di_mem_chunk *mem_chunk)
{
  void *mem;

  if ((!mem_chunk->mem_area) || ((mem_chunk->mem_area->index + mem_chunk->atom_size) > mem_chunk->area_size))
  {
    mem_chunk->mem_area = di_malloc (mem_chunk->rarea_size);

    mem_chunk->num_mem_areas += 1;
    mem_chunk->mem_area->next = mem_chunk->mem_areas;
    mem_chunk->mem_area->prev = NULL;

    if (mem_chunk->mem_areas)
      mem_chunk->mem_areas->prev = mem_chunk->mem_area;
    mem_chunk->mem_areas = mem_chunk->mem_area;

    mem_chunk->mem_area->index = 0;
    mem_chunk->mem_area->free = mem_chunk->area_size;
    mem_chunk->mem_area->allocated = 0;
  }

  mem = &mem_chunk->mem_area->mem[mem_chunk->mem_area->index];
  mem_chunk->mem_area->index += mem_chunk->atom_size;
  mem_chunk->mem_area->free -= mem_chunk->atom_size;
  mem_chunk->mem_area->allocated += 1;

  return mem;
}

/**
 * Allocate a cleared piece
 *
 * @param mem_chunk a di_mem_chunk
 *
 * @return memory
 */
void *di_mem_chunk_alloc0 (di_mem_chunk *mem_chunk)
{
  void *mem;

  mem = di_mem_chunk_alloc (mem_chunk);

  if (mem)
    memset (mem, 0, mem_chunk->atom_size);

  return mem;
}

void di_mem_chunk_destroy (di_mem_chunk *mem_chunk)
{
  di_mem_area *mem_areas, *temp_area;

  mem_areas = mem_chunk->mem_areas;
  while (mem_areas)
  {
    temp_area = mem_areas;
    mem_areas = mem_areas->next;
    di_free (temp_area);
  }

  di_free (mem_chunk);
}

size_t di_mem_chunk_size (di_mem_chunk *mem_chunk)
{
  di_mem_area *mem_area;
  size_t size = 0;

  for (mem_area = mem_chunk->mem_areas; mem_area; mem_area = mem_area->next)
  {
    size += mem_chunk->atom_size * mem_area->allocated;
  }

  return size;
}

static size_t di_mem_chunk_compute_size (di_ksize_t size, di_ksize_t min_size)
{
  di_ksize_t power_of_2;
  di_ksize_t lower, upper;

  power_of_2 = 16;
  while (power_of_2 < size)
    power_of_2 <<= 1;

  lower = power_of_2 >> 1;
  upper = power_of_2;

  if (size - lower < upper - size && lower >= min_size)
    return lower;
  else
    return upper;
}


