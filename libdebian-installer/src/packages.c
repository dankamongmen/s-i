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
 * $Id: packages.c,v 1.14 2004/02/27 23:22:00 waldi Exp $
 */

#include <config.h>

#include <debian-installer/packages_internal.h>

#include <debian-installer/log.h>
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
  ret->slist_node_mem_chunk = di_mem_chunk_new (sizeof (di_slist_node), 4096);

  return ret;
}

/**
 * Free di_packages
 */
void di_packages_free (di_packages *packages)
{
  if (!packages)
    return;
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
  di_mem_chunk_destroy (allocator->slist_node_mem_chunk);
  di_free (allocator);
}

/**
 * append a package.
 *
 * @param packages a di_packages
 */
void di_packages_append_package (di_packages *packages, di_package *package, di_packages_allocator *allocator)
{
  di_package *tmp;

  tmp = di_packages_get_package (packages, package->package, 0);

  if (!tmp)
    di_slist_append_chunk (&packages->list, package, allocator->slist_node_mem_chunk);

  di_hash_table_insert (packages->table, &package->key, package);
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

  /* i know that is bad, but i know it is not written by the lookup */
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
    ret = di_package_alloc (allocator);
    ret->key.string = di_stradup (name, n);
    ret->key.size = n;

    di_hash_table_insert (packages->table, &ret->key, ret);
  }

  return ret;
}

static bool resolve_dependencies_recurse (di_slist *install, di_package *package, di_package *dependend_package, di_packages_allocator *allocator, unsigned int resolver, bool shutup)
{
  di_slist_node *node, *node2;
  di_package *best_provide;

  /* did we already check this package? */
  if (!(package->resolver & resolver))
  {
    package->resolver |= resolver;

    switch (package->type)
    {
      case di_package_type_real_package:
        for (node = package->depends.head; node; node = node->next)
        {
          di_package_dependency *d = node->data;

          /* it's a dependency */
          if (d->type == di_package_dependency_type_depends || d->type == di_package_dependency_type_pre_depends)
          {
            /* ugh, someone don't respect our policy */
            if (!shutup && package->priority > d->ptr->priority)
              di_log (DI_LOG_LEVEL_INFO, "broken dependency: %s to %s (wrong priority)", package->package, d->ptr->package);
            /* check recursive */
            if (!resolve_dependencies_recurse (install, d->ptr, package, allocator, resolver, shutup))
              return false;
          }
          else if (d->type == di_package_dependency_type_conflicts)
            switch (d->ptr->type)
            {
              case di_package_type_real_package:
                if (d->ptr->status == di_package_status_unpacked ||
                    d->ptr->status == di_package_status_installed)
                  return false;
                break;
              case di_package_type_virtual_package:
                for (node2 = d->ptr->depends.head; node2; node2 = node2->next)
                {
                  di_package_dependency *d = node2->data;

                  if (d->type == di_package_dependency_type_reverse_provides)
                    if ((d->ptr->status == di_package_status_unpacked ||
                         d->ptr->status == di_package_status_installed) &&
                        d->ptr != package)
                      return false;
                }
                break;
              default:
                break;
            }
        }

        if (dependend_package)
          di_log (DI_LOG_LEVEL_DEBUG, "install %s, dependency from %s", package->package, dependend_package->package);

        if (install)
          di_slist_append_chunk (install, package, allocator->slist_node_mem_chunk);
        else
          package->status_want = di_package_status_want_install;
        break;

      case di_package_type_virtual_package:

        if (dependend_package)
          di_log (DI_LOG_LEVEL_DEBUG, "search for package resolving %s, dependency from %s", package->package, dependend_package->package);

        for (node = package->depends.head; node; node = node->next)
        {
          di_package_dependency *d = node->data;

          if (d->type == di_package_dependency_type_reverse_provides)
            package->resolver &= ~(resolver << 2);
        }

        while (1)
        {
          best_provide = NULL;

          for (node = package->depends.head; node; node = node->next)
          {
            di_package_dependency *d = node->data;

            if (d->type == di_package_dependency_type_reverse_provides)
            {
              if (!(package->resolver & (resolver << 2)) &&
                  (!best_provide || best_provide->priority < d->ptr->priority ||
                   (d->ptr->status == di_package_status_installed && best_provide->status != di_package_status_installed)))
                best_provide = d->ptr;
            }
          }

          if (best_provide)
          {
            if (resolve_dependencies_recurse (install, best_provide, package, allocator, resolver, shutup))
              break;
            else
              package->resolver |= (resolver << 2);
          }
          else
            return false;
        }

        break;

      case di_package_type_non_existent:
        if (!shutup)
          di_log (DI_LOG_LEVEL_WARNING, "package %s doesn't exist", package->package);
        return false;
    }

    package->resolver |= (resolver << 1);
    return true;
  }

  return package->resolver & (resolver << 1);
}

static void resolve_dependencies_marker_reset (void *key __attribute__ ((unused)), void *value, void *user_data __attribute__ ((unused)))
{
  di_package *p = value;
  p->resolver = 0;
}

static void resolve_dependencies_marker (di_packages *packages)
{
  if (!packages->resolver)
    packages->resolver = 1;
  else if (packages->resolver > (INT_MAX >> 2))
  {
    di_hash_table_foreach (packages->table, resolve_dependencies_marker_reset, NULL);
    packages->resolver = 1;
  }
  else
    packages->resolver <<= 3;

}

di_slist *di_packages_resolve_dependencies (di_packages *packages, di_slist *list, di_packages_allocator *allocator)
{
  di_slist *install = di_slist_alloc ();
  di_slist_node *node;

  resolve_dependencies_marker (packages);

  for (node = list->head; node; node = node->next)
  {
    di_package *p = node->data;
    if (!resolve_dependencies_recurse (install, p, NULL, allocator, packages->resolver, false))
    {
      di_slist_free (install);
      return NULL;
    }
  }

  return install;
}

di_slist *di_packages_resolve_dependencies_array (di_packages *packages, di_package **array, di_packages_allocator *allocator)
{
  di_slist *install = di_slist_alloc ();

  resolve_dependencies_marker (packages);

  while (*array)
    if (!resolve_dependencies_recurse (install, *array++, NULL, allocator, packages->resolver, false))
    {
      di_slist_free (install);
      return NULL;
    }

  return install;
}

void di_packages_resolve_dependencies_mark (di_packages *packages)
{
  di_slist_node *node;

  resolve_dependencies_marker (packages);

  for (node = packages->list.head; node; node = node->next)
  {
    di_package *package = node->data;
    if (!(package->resolver & packages->resolver) && package->status_want == di_package_status_want_install)
      resolve_dependencies_recurse (NULL, package, NULL, NULL, packages->resolver, true);
  }
}

