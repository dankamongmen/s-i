/*
 * packages.c
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
 * $Id: packages.c,v 1.4 2003/09/24 11:49:52 waldi Exp $
 */

#include <debian-installer/packages_internal.h>

#include <debian-installer/package_internal.h>
#include <debian-installer/string.h>

#include <ctype.h>
#include <limits.h>

/**
 * Allocate di_packages
 */
di_packages *di_packages_alloc (void)
{
  di_packages *ret;

  ret = di_new0 (di_packages, 1);
  ret->table = di_hash_table_new_full (di_rstring_hash, di_rstring_equal, NULL, internal_di_package_destroy_func);

  return ret;
}

/**
 * Allocate di_packages_allocator
 */
di_packages_allocator *di_packages_allocator_alloc (void)
{
  di_packages_allocator *ret;

  ret = internal_di_packages_allocator_alloc ();
  ret->package_mem_chunk = di_mem_chunk_new (sizeof (di_package), 16384);

  return ret;
}

/**
 * @internal
 * Partially allocate di_packages_allocator
 */
di_packages_allocator *internal_di_packages_allocator_alloc (void)
{
  di_packages_allocator *ret;

  ret = di_new0 (di_packages_allocator, 1);
  ret->package_dependency_mem_chunk = di_mem_chunk_new (sizeof (di_package_dependency), 4096);
  ret->package_description_mem_chunk = di_mem_chunk_new (sizeof (di_package_description), 4096);
  ret->slist_node_mem_chunk = di_mem_chunk_new (sizeof (di_slist_node), 4096);

  return ret;
}

/**
 * Free di_packages
 */
void di_packages_free (di_packages *packages)
{
  di_hash_table_destroy (packages->table);
  di_free (packages);
}

/**
 * Free di_packages_allocator
 */
void di_packages_allocator_free (di_packages_allocator *allocator)
{
  di_mem_chunk_destroy (allocator->package_mem_chunk);
  di_mem_chunk_destroy (allocator->package_dependency_mem_chunk);
  di_mem_chunk_destroy (allocator->package_description_mem_chunk);
  di_mem_chunk_destroy (allocator->slist_node_mem_chunk);
  di_free (allocator);
}

/**
 * get a named package.
 *
 * @param packages a di_packages
 * @param name the name of the package
 * @param n size of the name or 0
 *
 * @return the package or NULL
 */
di_package *di_packages_get_package (di_packages *packages, const char *name, size_t n)
{
  di_rstring key;
  size_t size;

  if (n)
    size = n;
  else
    size = strlen (name);

  key.string = (char *) name;
  key.size = size;

  return di_hash_table_lookup (packages->table, &key);
}

/**
 * get a named package.
 * creates a new one if non-existant.
 *
 * @param packages a di_packages
 * @param name the name of the package
 * @param n size of the name
 *
 * @return the package
 */
di_package *di_packages_get_package_new (di_packages *packages, di_packages_allocator *allocator, char *name, size_t n)
{
  di_package *ret = di_packages_get_package (packages, name, n);

  if (!ret)
  {
    char *temp;
    temp = di_stradup (name, n);
    ret = di_mem_chunk_alloc0 (allocator->package_mem_chunk);
    ret->key.string = temp;
    ret->key.size = n;

    di_hash_table_insert (packages->table, &ret->key, ret);
  }

  return ret;
}

static void resolve_dependencies_recurse (di_slist *install, di_package *package, di_packages_allocator *allocator, unsigned int resolver)
{
  di_slist_node *node;
  di_package *last_provide;

  if (!(package->resolver & resolver))
  {
    package->resolver |= resolver;

    switch (package->type)
    {
      case di_package_type_real_package:
        for (node = package->depends.first; node; node = node->next)
        {
          di_package_dependency *d = node->data;

          if (d->type == di_package_dependency_type_depends || d->type == di_package_dependency_type_pre_depends)
          {
#if 1
            if (package->priority > d->ptr->priority)
              di_log (DI_LOG_LEVEL_INFO, "broken dependency: %s to %s\n", package->key.string, d->ptr->key.string);
#endif
            resolve_dependencies_recurse (install, d->ptr, allocator, resolver);
          }
        }

        if (install)
          di_slist_append_chunk (install, package, allocator->slist_node_mem_chunk);
        else
          package->status_want = di_package_status_want_install;
        break;

      case di_package_type_virtual_package:
        last_provide = NULL;

        for (node = package->depends.first; node; node = node->next)
        {
          di_package_dependency *d = node->data;

          if (d->type == di_package_dependency_type_reverse_provides)
          {
            if (!last_provide || last_provide->priority < d->ptr->priority)
              last_provide = d->ptr;
          }
        }

        if (last_provide)
          resolve_dependencies_recurse (install, last_provide, allocator, resolver);
        break;

      case di_package_type_non_existent:
        di_log (DI_LOG_LEVEL_WARNING, "package %s don't exists\n", package->key.string);
        break;
    }
  }
}

static void resolve_dependencies_marker (di_packages *packages)
{
  if (!packages->resolver)
    packages->resolver = 1;
  else if (packages->resolver > INT_MAX)
  {
    di_slist_node *node;
    for (node = packages->list.first; node; node = node->next)
    {
      di_package *p = node->data;
      p->resolver = 0;
    }
    packages->resolver = 1;
  }
  else
    packages->resolver <<= 1;

}

di_slist *di_packages_resolve_dependencies (di_packages *packages, di_slist *list, di_packages_allocator *allocator)
{
  di_slist *install = di_slist_alloc ();
  di_slist_node *node;

  resolve_dependencies_marker (packages);

  for (node = list->first; node; node = node->next)
  {
    di_package *p = node->data;
    resolve_dependencies_recurse (install, p, allocator, packages->resolver);
  }

  return install;
}

void di_packages_resolve_dependencies_mark (di_packages *packages)
{
  di_slist_node *node;

  resolve_dependencies_marker (packages);

  for (node = packages->list.first; node; node = node->next)
  {
    di_package *package = node->data;
    if (!(package->resolver & packages->resolver) && package->status_want == di_package_status_want_install)
      resolve_dependencies_recurse (NULL, package, NULL, packages->resolver);
  }
}

