/*
 * hash.c
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
 * $Id: hash.c,v 1.4 2003/09/29 14:08:48 waldi Exp $
 */

#include <debian-installer/hash.h>

#include <string.h>

typedef struct di_hash_node di_hash_node;

/**
 * @addtogroup di_hash_table
 * @{
 */

/**
 * @internal
 * @brief Hash table
 */
struct di_hash_table
{
  di_ksize_t size;                                      /**< the overall size */
  di_ksize_t nnodes;                                    /**< number of nodes */
  di_hash_node **nodes;                                 /**< nodes */
  di_mem_chunk *mem_chunk;                              /**< di_mem_chunk for allocating the nodes (di_hash_node) */
  di_hash_func *hash_func;                              /**< hashing function */
  di_equal_func *key_equal_func;                        /**< key compare function */
  di_destroy_notify *key_destroy_func;                  /**< key destroy function, may NULL */
  di_destroy_notify *value_destroy_func;                /**< value destroy function, may NULL */
};

/**
 * @internal
 * @brief Node of a hash table
 */
struct di_hash_node
{
  void *key;                                            /**< key */
  void *value;                                          /**< value */
  di_hash_node *next;                                   /**< the next node */
};

/**
 * @internal
 * Defines if a resize is necessary
 *
 * @param hash_table a di_hash_table
 */
#define HASH_TABLE_RESIZE(hash_table)                   \
if ((hash_table->size >= 3 * hash_table->nnodes &&      \
      hash_table->size > HASH_TABLE_MIN_SIZE) ||        \
    (3 * hash_table->size <= hash_table->nnodes &&      \
     hash_table->size < HASH_TABLE_MAX_SIZE))           \
     di_hash_table_resize (hash_table);

/**
 * @internal
 * The minimal hash table size
 */
#define HASH_TABLE_MIN_SIZE 11
/**
 * @internal
 * The maximal hash table size
 */
#define HASH_TABLE_MAX_SIZE 13845163

/**
 * @internal
 * @param x value
 * @param low low bound
 * @param high high bound
 * 
 * @return a value between low and high
 */
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static void di_hash_table_resize (di_hash_table *hash_table);
static di_hash_node **di_hash_table_lookup_node (di_hash_table *hash_table, const void *key);
static di_hash_node *di_hash_node_new (di_hash_table *hash_table, void *key, void *value);
static void di_hash_node_destroy (di_hash_node *hash_node, di_destroy_notify key_destroy_func, di_destroy_notify value_destroy_func) __attribute__ ((unused));
static void di_hash_nodes_destroy (di_hash_node *hash_node, di_destroy_notify key_destroy_func, di_destroy_notify value_destroy_func);

static unsigned int di_spaced_primes_closest (unsigned int num);

/** @} */

/**
 * Creates a new di_hash_table.
 *
 * @param hash_func a function to create a hash value from a key.
 *   Hash values are used to determine where keys are stored within the
 *   di_hash_table data structure.
 * @param key_equal_func a function to check two keys for equality.  This is
 *   used when looking up keys in the di_hash_table.
 *
 * @return a new di_hash_table.
 **/
di_hash_table *di_hash_table_new (di_hash_func hash_func, di_equal_func key_equal_func)
{
  return di_hash_table_new_full (hash_func, key_equal_func, NULL, NULL);
}

/**
 * Creates a new di_hash_table like di_hash_table_new() and allows to specify
 * functions to free the memory allocated for the key and value that get
 * called when removing the entry from the di_hash_table
 *
 * @param hash_func a function to create a hash value from a key.
 *   Hash values are used to determine where keys are stored within the
 *   di_hash_table data structure.
 * @param key_equal_func a function to check two keys for equality.  This is
 *   used when looking up keys in the di_hash_table.
 * @param key_destroy_func a function to free the memory allocated for the key
 *   used when removing the entry from the di_hash_table or %NULL if you
 *   don't want to supply such a function.
 * @param value_destroy_func a function to free the memory allocated for the
 *   value used when removing the entry from the di_hash_table or %NULL if
 *   you don't want to supply such a function.
 *
 * @return a new di_hash_table.
 **/
di_hash_table *di_hash_table_new_full (di_hash_func hash_func, di_equal_func key_equal_func, di_destroy_notify key_destroy_func, di_destroy_notify value_destroy_func)
{
  di_hash_table *hash_table;
  di_ksize_t i;

  hash_table = di_new (di_hash_table, 1);
  hash_table->size               = HASH_TABLE_MIN_SIZE;
  hash_table->nnodes             = 0;
  hash_table->mem_chunk          = di_mem_chunk_new (sizeof (di_hash_node), 4096);
  hash_table->hash_func          = hash_func;
  hash_table->key_equal_func     = key_equal_func;
  hash_table->key_destroy_func   = key_destroy_func;
  hash_table->value_destroy_func = value_destroy_func;
  hash_table->nodes              = di_new (di_hash_node*, hash_table->size);

  for (i = 0; i < hash_table->size; i++)
    hash_table->nodes[i] = NULL;

  return hash_table;
}

/**
 * Destroys the di_hash_table. If keys and/or values are dynamically
 * allocated, you should either free them first or create the di_hash_table
 * using di_hash_table_new_full(). In the latter case the destroy functions
 * you supplied will be called on all keys and values before destroying
 * the di_hash_table.
 *
 * @param hash_table a di_hash_table.
 */
void di_hash_table_destroy (di_hash_table *hash_table)
{
  di_ksize_t i;

  for (i = 0; i < hash_table->size; i++)
    di_hash_nodes_destroy (hash_table->nodes[i], hash_table->key_destroy_func, hash_table->value_destroy_func);

  di_mem_chunk_destroy (hash_table->mem_chunk);

  di_free (hash_table->nodes);
  di_free (hash_table);
}

static inline di_hash_node** di_hash_table_lookup_node (di_hash_table *hash_table, const void *key)
{
  di_hash_node **node;

  node = &hash_table->nodes
    [hash_table->hash_func (key) % hash_table->size];

  /* Hash table lookup needs to be fast.
   *  We therefore remove the extra conditional of testing
   *  whether to call the key_equal_func or not from
   *  the inner loop.
   */
  if (hash_table->key_equal_func)
    while (*node && !(*hash_table->key_equal_func) ((*node)->key, key))
      node = &(*node)->next;
  else
    while (*node && (*node)->key != key)
      node = &(*node)->next;

  return node;
}

/**
 * Looks up a key in a di_hash_table.
 *
 * @param hash_table a di_hash_table,
 * @param key the key to look up.
 *
 * @return the associated value, or %NULL if the key is not found.
 */
void *di_hash_table_lookup (di_hash_table *hash_table, const void *key)
{
  di_hash_node *node;

  node = *di_hash_table_lookup_node (hash_table, key);

  return node ? node->value : NULL;
}

/**
 * Inserts a new key and value into a di_hash_table.
 *
 * If the key already exists in the di_hash_table its current value is replaced
 * with the new value. If you supplied a value_destroy_func when creating the
 * di_hash_table, the old value is freed using that function. If you supplied
 * a key_destroy_func when creating the di_hash_table, the passed key is freed
 * using that function.
 *
 * @param hash_table a di_hash_table.
 * @param key a key to insert.
 * @param value the value to associate with the key.
 */
void di_hash_table_insert (di_hash_table *hash_table, void *key, void *value)
{
  di_hash_node **node;

  node = di_hash_table_lookup_node (hash_table, key);

  if (*node)
  {
    if (hash_table->key_destroy_func)
      hash_table->key_destroy_func (key);

    if (hash_table->value_destroy_func)
      hash_table->value_destroy_func ((*node)->value);

    (*node)->value = value;
  }
  else
  {
    *node = di_hash_node_new (hash_table, key, value);
    hash_table->nnodes++;
    HASH_TABLE_RESIZE (hash_table);
  }
}

static di_hash_node* di_hash_node_new (di_hash_table *hash_table, void *key, void *value)
{
  di_hash_node *hash_node;

  hash_node = di_mem_chunk_alloc (hash_table->mem_chunk);

  hash_node->key = key;
  hash_node->value = value;
  hash_node->next = NULL;

  return hash_node;
}

static void di_hash_node_destroy (di_hash_node *hash_node, di_destroy_notify key_destroy_func, di_destroy_notify value_destroy_func)
{
  if (key_destroy_func)
    key_destroy_func (hash_node->key);
  if (value_destroy_func)
    value_destroy_func (hash_node->value);
}

static void di_hash_nodes_destroy (di_hash_node *hash_node, di_destroy_notify key_destroy_func, di_destroy_notify value_destroy_func)
{
  if (hash_node)
  {
    di_hash_node *node = hash_node;

    while (node->next)
    {
      if (key_destroy_func)
        key_destroy_func (node->key);
      if (value_destroy_func)
        value_destroy_func (node->value);

      node = node->next;
    }

    if (key_destroy_func)
      key_destroy_func (node->key);
    if (value_destroy_func)
      value_destroy_func (node->value);
  }
}

/**
 * Calls the given function for each of the key/value pairs in the
 * di_hash_table. The function is passed the key and value of each
 * pair, and the given user_data parameter.
 *
 * @post The hash table may not be modified while iterating over it
 * (you can't add/remove items).
 *
 * @param hash_table a di_hash_table.
 * @param func the function to call for each key/value pair.
 * @param user_data user data to pass to the function.
 */
void di_hash_table_foreach (di_hash_table *hash_table, di_hfunc *func, void *user_data)
{
  di_hash_node *node;
  size_t i;

  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = node->next)
      func (node->key, node->value, user_data);
}

di_ksize_t di_hash_table_size (di_hash_table *hash_table)
{
  return hash_table->size;
}

static void di_hash_table_resize (di_hash_table *hash_table)
{
  di_hash_node **new_nodes;
  di_hash_node *node;
  di_hash_node *next;
  uint32_t hash_val;
  size_t new_size;
  size_t i;

  new_size = di_spaced_primes_closest (hash_table->nnodes);
  new_size = CLAMP (new_size, HASH_TABLE_MIN_SIZE, HASH_TABLE_MAX_SIZE);

  new_nodes = di_new0 (di_hash_node*, new_size);

  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = next)
    {
      next = node->next;

      hash_val = (* hash_table->hash_func) (node->key) % new_size;

      node->next = new_nodes[hash_val];
      new_nodes[hash_val] = node;
    }

  di_free (hash_table->nodes);
  hash_table->nodes = new_nodes;
  hash_table->size = new_size;
}

static const unsigned int di_primes[] =
{
  11,
  19,
  37,
  73,
  109,
  163,
  251,
  367,
  557,
  823,
  1237,
  1861,
  2777,
  4177,
  6247,
  9371,
  14057,
  21089,
  31627,
  47431,
  71143,
  106721,
  160073,
  240101,
  360163,
  540217,
  810343,
  1215497,
  1823231,
  2734867,
  4102283,
  6153409,
  9230113,
  13845163,
};

static const unsigned int di_nprimes = sizeof (di_primes) / sizeof (di_primes[0]);

static unsigned int di_spaced_primes_closest (unsigned int num)
{
  unsigned int i;

  for (i = 0; i < di_nprimes; i++)
    if (di_primes[i] > num)
      return di_primes[i];

  return di_primes[di_nprimes - 1];
}

