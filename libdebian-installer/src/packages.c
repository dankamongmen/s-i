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
 * $Id: packages.c,v 1.3 2003/09/15 20:02:47 waldi Exp $
 */

#include <debian-installer/packages.h>
#include <debian-installer/packages_internal.h>

#include <debian-installer/parser_rfc822.h>
#include <debian-installer/string.h>

#include <ctype.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @addtogroup di_packages_parser
 * @{
 */
/**
 * @internal
 * parser info
 */
const di_parser_fieldinfo 
  internal_di_packages_parser_field_package = 
    DI_PARSER_FIELDINFO
    (
      "Package",
      di_packages_parser_read_name,
      di_parser_write_string,
      offsetof (di_package, key.string)
    ),
  internal_di_packages_parser_field_package_string = 
    DI_PARSER_FIELDINFO
    (
      "Package",
      di_parser_read_string,
      di_parser_write_string,
      offsetof (di_package, key.string)
    ),
  internal_di_packages_parser_field_status =
    DI_PARSER_FIELDINFO
    (
      "Status",
      di_packages_parser_read_status,
      di_packages_parser_write_status,
      0
    ),
  internal_di_packages_parser_field_essential =
    DI_PARSER_FIELDINFO
    (
      "Essential",
      di_parser_read_boolean,
      di_parser_write_boolean,
      offsetof (di_package, essential)
    ),
  internal_di_packages_parser_field_priority =
    DI_PARSER_FIELDINFO
    (
      "Priority",
      di_packages_parser_read_priority,
      di_packages_parser_write_priority,
      0
    ),
  internal_di_packages_parser_field_section =
    DI_PARSER_FIELDINFO
    (
      "Section",
      di_parser_read_string,
      di_parser_write_string,
      offsetof (di_package, section)
    ),
  internal_di_packages_parser_field_installed_size =
    DI_PARSER_FIELDINFO
    (
      "Installed-Size",
      di_parser_read_int,
      di_parser_write_int,
      offsetof (di_package, installed_size)
    ),
  internal_di_packages_parser_field_maintainer =
    DI_PARSER_FIELDINFO
    (
      "Maintainer",
      di_parser_read_string,
      di_parser_write_string,
      offsetof (di_package, maintainer)
    ),
  internal_di_packages_parser_field_architecture =
    DI_PARSER_FIELDINFO
    (
      "Architecture",
      di_parser_read_string,
      di_parser_write_string,
      offsetof (di_package, architecture)
    ),
  internal_di_packages_parser_field_version =
    DI_PARSER_FIELDINFO
    (
      "Version",
      di_parser_read_string,
      di_parser_write_string,
      offsetof (di_package, version)
    ),
  internal_di_packages_parser_field_replaces =
    DI_PARSER_FIELDINFO
    (
      "Replaces",
      di_packages_parser_read_dependency,
      di_packages_parser_write_dependency,
      di_package_dependency_type_replaces
    ),
  internal_di_packages_parser_field_provides =
    DI_PARSER_FIELDINFO
    (
      "Provides",
      di_packages_parser_read_dependency,
      di_packages_parser_write_dependency,
      di_package_dependency_type_provides
    ),
  internal_di_packages_parser_field_depends =
    DI_PARSER_FIELDINFO
    (
      "Depends",
      di_packages_parser_read_dependency,
      di_packages_parser_write_dependency,
      di_package_dependency_type_depends
    ),
  internal_di_packages_parser_field_pre_depends =
    DI_PARSER_FIELDINFO
    (
      "Pre-Depends",
      di_packages_parser_read_dependency,
      di_packages_parser_write_dependency,
      di_package_dependency_type_pre_depends
    ),
  internal_di_packages_parser_field_recommends =
    DI_PARSER_FIELDINFO
    (
      "Recommends",
      di_packages_parser_read_dependency,
      di_packages_parser_write_dependency,
      di_package_dependency_type_recommends
    ),
  internal_di_packages_parser_field_suggests =
    DI_PARSER_FIELDINFO
    (
      "Suggests",
      di_packages_parser_read_dependency,
      di_packages_parser_write_dependency,
      di_package_dependency_type_suggests
    ),
  internal_di_packages_parser_field_conflicts =
    DI_PARSER_FIELDINFO
    (
      "Conflicts",
      di_packages_parser_read_dependency,
      di_packages_parser_write_dependency,
      di_package_dependency_type_conflicts
    ),
  internal_di_packages_parser_field_enhances =
    DI_PARSER_FIELDINFO
    (
      "Enhances",
      di_packages_parser_read_dependency,
      di_packages_parser_write_dependency,
      di_package_dependency_type_enhances
    ),
  internal_di_packages_parser_field_filename =
    DI_PARSER_FIELDINFO
    (
      "Filename",
      di_parser_read_string,
      di_parser_write_string,
      offsetof (di_package, filename)
    ),
  internal_di_packages_parser_field_size =
    DI_PARSER_FIELDINFO
    (
      "Size",
      di_parser_read_int,
      di_parser_write_int,
      offsetof (di_package, size)
    ),
  internal_di_packages_parser_field_md5sum =
    DI_PARSER_FIELDINFO
    (
      "MD5sum",
      di_parser_read_string,
      di_parser_write_string,
      offsetof (di_package, md5sum)
    ),
  internal_di_packages_parser_field_description =
    DI_PARSER_FIELDINFO
    (
      "Description",
      di_packages_parser_read_description,
      NULL,
      0
    );

/**
 * Standard Packages file
 */
const di_parser_fieldinfo *di_packages_parser_fieldinfo[] =
{
  &internal_di_packages_parser_field_package,
  &internal_di_packages_parser_field_essential,
  &internal_di_packages_parser_field_priority,
  &internal_di_packages_parser_field_section,
  &internal_di_packages_parser_field_installed_size,
  &internal_di_packages_parser_field_maintainer,
  &internal_di_packages_parser_field_architecture,
  &internal_di_packages_parser_field_version,
  &internal_di_packages_parser_field_replaces,
  &internal_di_packages_parser_field_provides,
  &internal_di_packages_parser_field_depends,
  &internal_di_packages_parser_field_pre_depends,
  &internal_di_packages_parser_field_recommends,
  &internal_di_packages_parser_field_suggests,
  &internal_di_packages_parser_field_conflicts,
  &internal_di_packages_parser_field_enhances,
  &internal_di_packages_parser_field_filename,
  &internal_di_packages_parser_field_size,
  &internal_di_packages_parser_field_md5sum,
  &internal_di_packages_parser_field_description,
  NULL
};

/**
 * Standard package control file
 */
const di_parser_fieldinfo *di_packages_control_parser_fieldinfo[] =
{
  &internal_di_packages_parser_field_package_string,
  &internal_di_packages_parser_field_essential,
  &internal_di_packages_parser_field_priority,
  &internal_di_packages_parser_field_section,
  &internal_di_packages_parser_field_installed_size,
  &internal_di_packages_parser_field_maintainer,
  &internal_di_packages_parser_field_architecture,
  &internal_di_packages_parser_field_version,
  &internal_di_packages_parser_field_replaces,
  &internal_di_packages_parser_field_provides,
  &internal_di_packages_parser_field_depends,
  &internal_di_packages_parser_field_pre_depends,
  &internal_di_packages_parser_field_recommends,
  &internal_di_packages_parser_field_suggests,
  &internal_di_packages_parser_field_conflicts,
  &internal_di_packages_parser_field_enhances,
  &internal_di_packages_parser_field_filename,
  &internal_di_packages_parser_field_size,
  &internal_di_packages_parser_field_md5sum,
  &internal_di_packages_parser_field_description,
  NULL
};

/**
 * Standard status file
 */
const di_parser_fieldinfo *di_packages_status_parser_fieldinfo[] =
{
  &internal_di_packages_parser_field_package,
  &internal_di_packages_parser_field_status,
  &internal_di_packages_parser_field_essential,
  &internal_di_packages_parser_field_priority,
  &internal_di_packages_parser_field_section,
  &internal_di_packages_parser_field_installed_size,
  &internal_di_packages_parser_field_maintainer,
  &internal_di_packages_parser_field_version,
  &internal_di_packages_parser_field_replaces,
  &internal_di_packages_parser_field_provides,
  &internal_di_packages_parser_field_depends,
  &internal_di_packages_parser_field_pre_depends,
  &internal_di_packages_parser_field_recommends,
  &internal_di_packages_parser_field_suggests,
  &internal_di_packages_parser_field_conflicts,
  &internal_di_packages_parser_field_enhances,
  &internal_di_packages_parser_field_description,
  NULL
};

/**
 * Minimal Packages file
 */
const di_parser_fieldinfo *di_packages_minimal_parser_fieldinfo[] =
{
  &internal_di_packages_parser_field_package,
  &internal_di_packages_parser_field_essential,
  &internal_di_packages_parser_field_installed_size,
  &internal_di_packages_parser_field_version,
  &internal_di_packages_parser_field_provides,
  &internal_di_packages_parser_field_depends,
  &internal_di_packages_parser_field_pre_depends,
  &internal_di_packages_parser_field_filename,
  &internal_di_packages_parser_field_md5sum,
  NULL
};

/** @} */

static void di_package_destroy_func (void *data)
{
  di_package *package = data;

  di_free (package->key.string);
  di_free (package->section);
  di_free (package->maintainer);
  di_free (package->architecture);
  di_free (package->version);
  di_free (package->filename);
  di_free (package->md5sum);
}

/**
 * Allocate di_packages
 */
di_packages *di_packages_alloc (void)
{
  di_packages *ret;

  ret = di_new0 (di_packages, 1);
  ret->table = di_hash_table_new_full (di_rstring_hash, di_rstring_equal, NULL, di_package_destroy_func);

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
  ret->package_dependency_group_mem_chunk = di_mem_chunk_new (sizeof (di_package_dependency_group), 4096);
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
  di_mem_chunk_destroy (allocator->package_dependency_group_mem_chunk);
  di_mem_chunk_destroy (allocator->package_description_mem_chunk);
  di_mem_chunk_destroy (allocator->slist_node_mem_chunk);
  di_free (allocator);
}

/**
 * @internal
 * Get parser info for standard Packages file
 */
di_parser_info *internal_di_packages_parser_info (void)
{
  di_parser_info *info;

  info = di_parser_info_alloc ();
  di_parser_info_add_pointer (info, di_packages_parser_fieldinfo);

  return info;
}

static void *control_parser_new (void *user_data)
{
  di_packages_parser_data *parser_data = user_data;
  return parser_data->package;
}

/**
 * @internal
 * Get parser info for standard packages control file
 */
di_parser_info *internal_di_packages_control_parser_info (void)
{
  di_parser_info *info;

  info = di_parser_info_alloc ();
  di_parser_info_add_pointer (info, di_packages_control_parser_fieldinfo);

  return info;
}

/**
 * @internal
 * Get parser info for standard status file
 */
di_parser_info *internal_di_packages_status_parser_info (void)
{
  di_parser_info *info;

  info = di_parser_info_alloc ();
  di_parser_info_add_pointer (info, di_packages_status_parser_fieldinfo);

  return info;
}

/**
 * Read a standard Packages file
 *
 * @param file file to read
 */
di_packages *di_packages_read_file (const char *file, di_packages_allocator *allocator)
{
  di_packages *packages;
  di_parser_info *info;

  info = internal_di_packages_parser_info ();
  packages = di_packages_read_file_special (file, allocator, info);
  di_parser_info_free (info);

  return packages;
}

/**
 * Read a special Packages file
 *
 * @param file file to read
 * @param info parser info
 */
di_packages *di_packages_read_file_special (const char *file, di_packages_allocator *allocator, di_parser_info *info)
{
  di_packages_parser_data data;

  data.allocator = allocator;
  data.packages = di_packages_alloc ();

  if (di_parser_rfc822_read_file (file, info, NULL, NULL, &data) < 0)
  {
    di_packages_free (data.packages);
    return NULL;
  }

  return data.packages;
}

/**
 * Read a package control file
 *
 * @param file file to read
 */
di_package *di_packages_control_read_file_special (const char *file, di_packages_allocator *allocator)
{
  di_packages_parser_data data;
  di_parser_info *info;

  data.allocator = allocator;
  data.package = di_mem_chunk_alloc (allocator->package_mem_chunk);

  info = internal_di_packages_control_parser_info ();

  if (di_parser_rfc822_read_file (file, info, NULL, NULL, &data) < 0)
  {
    di_parser_info_free (info);
    di_packages_free (data.packages);
    return NULL;
  }

  di_parser_info_free (info);
  return data.package;
}

/**
 * Read a standard status file
 *
 * @param file file to read
 */
di_packages *di_packages_status_read_file (const char *file, di_packages_allocator *allocator)
{
  di_packages *packages;
  di_parser_info *info;

  info = internal_di_packages_status_parser_info ();
  packages = di_packages_read_file_special (file, allocator, info);
  di_parser_info_free (info);

  return packages;
}

/**
 * Write a standard Packages file
 *
 * @param packages a di_packages
 * @param file file to write
 *
 * @return number of dumped entries
 */
int di_packages_write_file (di_packages *packages, const char *file)
{
  di_parser_info *info;
  int ret;

  info = internal_di_packages_parser_info ();
  ret = di_parser_rfc822_write_file (file, info, internal_di_packages_parser_write_entry_next, packages);
  di_parser_info_free (info);

  return ret;
}

/**
 * Write a standard status file
 *
 * @param packages a di_packages
 * @param file file to write
 *
 * @return number of dumped entries
 */
int di_packages_status_write_file (di_packages *packages, const char *file)
{
  di_parser_info *info;
  int ret;

  info = internal_di_packages_status_parser_info ();
  ret = di_parser_rfc822_write_file (file, info, internal_di_packages_parser_write_entry_next, packages);
  di_parser_info_free (info);

  return ret;
}

void *internal_di_packages_parser_write_entry_next (void **state_data, void *user_data)
{
  if (!*state_data)
  {
    di_packages *p = user_data;
    *state_data = p->list.first;
    return p->list.first->data;
  }
  else
  {
    di_slist_node *n = *state_data;
    n = n->next;

    if (n)
    {
      *state_data = n;
      return n->data;
    }
    else
      return NULL;
  }
}

/**
 * Get a named package.
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
 * Get a named package.
 * Creates a new one if non-existant.
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

di_package_dependency *di_package_dependency_alloc (di_packages_allocator *allocator)
{
  di_package_dependency *ret;
  ret = di_mem_chunk_alloc0 (allocator->package_dependency_mem_chunk);
  return ret;
}

di_package_dependency_group *di_package_dependency_group_alloc (di_packages_allocator *allocator)
{
  di_package_dependency_group *ret;
  ret = di_mem_chunk_alloc0 (allocator->package_dependency_group_mem_chunk);
  return ret;
}

di_package_description *di_package_description_alloc (di_packages_allocator *allocator)
{
  di_package_description *ret;
  ret = di_mem_chunk_alloc0 (allocator->package_description_mem_chunk);
  return ret;
}

void di_packages_parser_read_dependency (data, fip, field_modifier, value, user_data)
  void **data;
  const di_parser_fieldinfo *fip __attribute__ ((unused));
  di_rstring *field_modifier __attribute__ ((unused));
  di_rstring *value;
  void *user_data __attribute__ ((unused));
{
  di_packages_parser_data *parser_data = user_data;
  di_package *p = *data, *q;
  char *cur = value->string, *end = value->string + value->size;
  char *namebegin, *fieldend;
  size_t namelen;
  di_package_dependency *d, *d1;
  di_package_dependency_group *g, *g1;

  /*
   * Basic depends line parser. Can ignore versioning
   * info since the depends are already satisfied.
   */
  while (cur < end)
  {
    namebegin = cur;
    namelen = strcspn (cur, " \t\n(,|");

    d = di_package_dependency_alloc (parser_data->allocator);
    g = di_package_dependency_group_alloc (parser_data->allocator);

    q = di_packages_get_package_new (parser_data->packages, parser_data->allocator, namebegin, namelen);

    d->ptr = q;
    g->type = fip->integer;
    di_slist_append_chunk (&g->list, d, parser_data->allocator->slist_node_mem_chunk);
    di_slist_append_chunk (&p->depends, g, parser_data->allocator->slist_node_mem_chunk);

    if (g->type == di_package_dependency_type_provides)
    {
      if (q->type == di_package_type_non_existent)
        q->type = di_package_type_virtual_package;
      if (q->type == di_package_type_virtual_package && q->priority < p->priority)
        q->priority = p->priority;

      d1 = di_package_dependency_alloc (parser_data->allocator);
      g1 = di_package_dependency_group_alloc (parser_data->allocator);

      d1->ptr = p;
      g1->type = di_package_dependency_type_reverse_provides;
      di_slist_append_chunk (&g1->list, d1, parser_data->allocator->slist_node_mem_chunk);
      di_slist_append_chunk (&q->depends, g1, parser_data->allocator->slist_node_mem_chunk);
    }

    fieldend = cur + strcspn (cur, "\n,");
    while (isspace(*++fieldend));
    cur = fieldend;
  }
}

void di_packages_parser_write_dependency (data, fip, callback, callback_data, user_data)
  void **data;
  const di_parser_fieldinfo *fip;
  di_parser_fields_function_write_callback callback;
  void *callback_data;
  void *user_data __attribute__ ((unused));
{
}

void di_packages_parser_read_description (data, fip, field_modifier, value, user_data)
  void **data;
  const di_parser_fieldinfo *fip __attribute__ ((unused));
  di_rstring *field_modifier __attribute__ ((unused));
  di_rstring *value;
  void *user_data __attribute__ ((unused));
{
  di_packages_parser_data *parser_data = user_data;
  di_package *p = *data;
  di_package_description *d = di_package_description_alloc (parser_data->allocator);
  char *temp;

  d->language = di_stradup (field_modifier->string, field_modifier->size);

  temp = memchr (value->string, '\n', value->size);
  if (temp)
  {
    d->short_description = di_stradup (value->string, temp - value->string - 1);
    d->description = di_stradup (temp, (value->string + value->size) - temp - 1);
  }
  else
    d->short_description = di_stradup (value->string, value->size);
}

void di_packages_parser_write_description (data, fip, callback, callback_data, user_data)
  void **data;
  const di_parser_fieldinfo *fip;
  di_parser_fields_function_write_callback callback;
  void *callback_data;
  void *user_data __attribute__ ((unused));
{
  di_packages_parser_data *parser_data = user_data;
  di_package *p = *data;
  di_slist_node *node;

  for (node = p->descriptions.first; node; node = node->next)
  {
  }
}

void di_packages_parser_read_name (data, fip, field_modifier, value, user_data)
  void **data;
  const di_parser_fieldinfo *fip __attribute__ ((unused));
  di_rstring *field_modifier __attribute__ ((unused));
  di_rstring *value;
  void *user_data __attribute__ ((unused));
{
  di_packages_parser_data *parser_data = user_data;
  di_package *p;
  p = di_packages_get_package_new (parser_data->packages, parser_data->allocator, value->string, value->size);
  p->type = di_package_type_real_package;
  di_slist_append_chunk (&parser_data->packages->list, p, parser_data->allocator->slist_node_mem_chunk);
  *data = p;
}

struct nr_to_string
{
  di_rstring string;
  unsigned int nr;
};
#define nr_to_string_one(string, nr) { {  string, sizeof (string) - 1 }, nr }

const struct nr_to_string priorities[] =
{
  nr_to_string_one ("required", di_package_priority_required),
  nr_to_string_one ("important", di_package_priority_important),
  nr_to_string_one ("standard", di_package_priority_standard),
  nr_to_string_one ("optional", di_package_priority_optional),
  nr_to_string_one ("extra", di_package_priority_extra),
  nr_to_string_one ("", 0),
};

void di_packages_parser_read_priority (data, fip, field_modifier, value, user_data)
  void **data;
  const di_parser_fieldinfo *fip __attribute__ ((unused));
  di_rstring *field_modifier __attribute__ ((unused));
  di_rstring *value;
  void *user_data __attribute__ ((unused));
{
  di_package *p = *data;
  const struct nr_to_string *prio;

  for (prio = priorities; prio->string.size; prio++)
    if (!strncmp (prio->string.string, value->string, prio->string.size))
    {
      p->priority = prio->nr;
      return;
    }
  p->priority = di_package_priority_extra;
}

void di_packages_parser_write_priority (data, fip, callback, callback_data, user_data)
  void **data;
  const di_parser_fieldinfo *fip;
  di_parser_fields_function_write_callback callback;
  void *callback_data;
  void *user_data __attribute__ ((unused));
{
  di_package *p = *data;
  const struct nr_to_string *prio;

  for (prio = priorities; prio->string.size; prio++)
    if (p->priority == prio->nr)
    {
      callback (&fip->key, &prio->string, callback_data);
      return;
    }
}

const struct nr_to_string status_want[] =
{
  nr_to_string_one ("unknown", di_package_status_want_unknown),
  nr_to_string_one ("install", di_package_status_want_install),
  nr_to_string_one ("hold", di_package_status_want_hold),
  nr_to_string_one ("deinstall", di_package_status_want_deinstall),
  nr_to_string_one ("purge", di_package_status_want_purge),
  nr_to_string_one ("", 0),
};

const struct nr_to_string status[] =
{
  nr_to_string_one ("not-installed", di_package_status_not_installed),
  nr_to_string_one ("unpacked", di_package_status_unpacked),
  nr_to_string_one ("half-configured", di_package_status_half_configured),
  nr_to_string_one ("installed", di_package_status_installed),
  nr_to_string_one ("half-configured", di_package_status_half_configured),
  nr_to_string_one ("config-files", di_package_status_config_files),
  nr_to_string_one ("", 0),
};

void di_packages_parser_read_status (data, fip, field_modifier, value, user_data)
  void **data;
  const di_parser_fieldinfo *fip __attribute__ ((unused));
  di_rstring *field_modifier __attribute__ ((unused));
  di_rstring *value;
  void *user_data __attribute__ ((unused));
{
  di_package *p = *data;
  const struct nr_to_string *stat;
  char *next;

  for (stat = status_want; stat->string.size; stat++)
    if (!strncmp (stat->string.string, value->string, stat->string.size))
    {
      p->status_want = stat->nr;
      break;
    }

  next = memchr (value->string, ' ', value->size);
  next = memchr (next + 1, ' ', value->size - (next - value->string) - 1);
  for (stat = status; stat->string.size; stat++)
    if (!strncmp (stat->string.string, next + 1, stat->string.size))
    {
      p->status = stat->nr;
      break;
    }
}

void di_packages_parser_write_status (data, fip, callback, callback_data, user_data)
  void **data;
  const di_parser_fieldinfo *fip;
  di_parser_fields_function_write_callback callback;
  void *callback_data;
  void *user_data __attribute__ ((unused));
{
  di_package *p = *data;
  const struct nr_to_string *stat;
  char value_buf[128];
  di_rstring value = { value_buf, 0 };

  for (stat = status_want; stat->string.size; stat++)
    if (p->status_want == stat->nr)
    {
      value.size = snprintf (value.string, sizeof (value_buf), "%s ok", stat->string.string);
      break;
    }
  for (stat = status; stat->string.size; stat++)
    if (p->status == stat->nr)
    {
      value.size += di_snprintfcat (value.string, sizeof (value_buf), " %s", stat->string.string);
      callback (&fip->key, &value, callback_data);
      return;
    }
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
          di_package_dependency_group *g = node->data;

          if (g->type == di_package_dependency_type_depends || g->type == di_package_dependency_type_pre_depends)
          {
            /* we don't parse more than one dependency per group, don't need to traverse the list */
            di_package_dependency *d = g->list.first->data;
#if 0
            if (package->priority > d->ptr->priority)
              fprintf (stderr, "broken dependency: %s to %s\n", package->key.string, d->ptr->key.string);
#endif
            resolve_dependencies_recurse (install, d->ptr, allocator, resolver);
          }
        }

        di_slist_append_chunk (install, package, allocator->slist_node_mem_chunk);
        break;

      case di_package_type_virtual_package:
        last_provide = NULL;

        for (node = package->depends.first; node; node = node->next)
        {
          di_package_dependency_group *g = node->data;

          if (g->type == di_package_dependency_type_reverse_provides)
          {
            /* we don't parse more than one dependency per group, don't need to traverse the list */
            di_package_dependency *d = g->list.first->data;
            if (!last_provide || last_provide->priority < d->ptr->priority)
              last_provide = d->ptr;
          }
        }

        if (last_provide)
          resolve_dependencies_recurse (install, last_provide, allocator, resolver);
        break;

      case di_package_type_non_existent:
        fprintf (stderr, "package %s don't exists\n", package->key.string);
        break;
    }
  }
}

di_slist *di_packages_resolve_dependencies (di_packages *packages, di_slist *list, di_packages_allocator *allocator)
{
  di_slist *install = di_slist_alloc ();
  di_slist_node *node;

  if (!packages->resolver)
    packages->resolver = 1;
  else if (packages->resolver > INT_MAX)
  {
    for (node = packages->list.first; node; node = node->next)
    {
      di_package *p = node->data;
      p->resolver = 0;
    }
    packages->resolver = 1;
  }
  else
    packages->resolver <<= 1;

  for (node = list->first; node; node = node->next)
  {
    di_package *p = node->data;
    resolve_dependencies_recurse (install, p, allocator, packages->resolver);
  }

  return install;
}

