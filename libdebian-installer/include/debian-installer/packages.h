/*
 * packages.h
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
 * $Id: packages.h,v 1.3 2003/09/15 20:02:46 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__PACKAGES_H
#define DEBIAN_INSTALLER__PACKAGES_H

#include <debian-installer/hash.h>
#include <debian-installer/parser.h>
#include <debian-installer/slist.h>

#include <stddef.h>
#include <string.h>

typedef struct di_package di_package;
typedef struct di_package_dependency di_package_dependency;
typedef struct di_package_dependency_group di_package_dependency_group;
typedef struct di_package_description di_package_description;
typedef struct di_packages di_packages;
typedef struct di_packages_allocator di_packages_allocator;
typedef struct di_packages_parser_data di_packages_parser_data;

typedef enum di_package_dependency_type di_package_dependency_type;
typedef enum di_package_priority di_package_priority;
typedef enum di_package_status di_package_status;
typedef enum di_package_status_want di_package_status_want;
typedef enum di_package_type di_package_type;

/**
 * @defgroup di_packages Packages file
 * @{
 */
/**
 * @defgroup di_packages_parser Packages file - Parser
 */

/**
 * Packages file - Priority field
 */
enum di_package_priority
{
  di_package_priority_extra = 1,
  di_package_priority_optional,
  di_package_priority_standard,
  di_package_priority_important,
  di_package_priority_required,
};

/**
 * Packages file - Status field, third part
 */
enum di_package_status
{
  di_package_status_undefined = 0,
  di_package_status_not_installed,
  di_package_status_unpacked,
  di_package_status_installed,
  di_package_status_half_configured,
  di_package_status_config_files,
};

/**
 * Packages file - Status field, first part
 */
enum di_package_status_want
{
  di_package_status_want_unknown = 0,
  di_package_status_want_install,
  di_package_status_want_hold,
  di_package_status_want_deinstall,
  di_package_status_want_purge,
};

/**
 * Packages file - type of package
 */
enum di_package_type
{
  di_package_type_non_existent = 0,                     /**< @internal */
  di_package_type_virtual_package,                      /**< Virtual package. */
  di_package_type_real_package,                         /**< Real package */
};

/**
 * @brief Packages file - Package
 */
struct di_package
{
  di_rstring key;                                       /**< Package field */
  di_package_type type;                                 /**< Type of package */
  di_package_status_want status_want;                   /**< Status field, first part */
  di_package_status status;                             /**< Status field, third part */
  int essential;                                        /**< Essential field */
  di_package_priority priority;                         /**< Priority field */
  char *section;                                        /**< Section field */
  int installed_size;                                   /**< Installed-Size field */
  char *maintainer;                                     /**< Maintainer field */
  char *architecture;                                   /**< Architecture field */
  char *version;                                        /**< Version field */
  di_slist depends;                                     /**< Any different dependency types */
  char *filename;                                       /**< Filename field */
  size_t size;                                          /**< Size field */
  char *md5sum;                                         /**< MD5Sum field */
  di_slist descriptions;                                /**< Description fields */
  unsigned int resolver;                                /**< @internal */
};

/**
 * Type of dependency
 */
enum di_package_dependency_type
{
  di_package_dependency_type_replaces,                  /**< Replaces field */
  di_package_dependency_type_provides,                  /**< Provides field */
  di_package_dependency_type_depends,                   /**< Depends field */
  di_package_dependency_type_pre_depends,               /**< Pre-Depends field */
  di_package_dependency_type_recommends,                /**< Recommends field */
  di_package_dependency_type_suggests,                  /**< Suggests field */
  di_package_dependency_type_conflicts,                 /**< Conflicts field */
  di_package_dependency_type_enhances,                  /**< Enhances field */
  di_package_dependency_type_reverse_provides = 0x100,  /**< @internal */
  di_package_dependency_type_reverse_enhances,          /**< @internal */
};

/**
 * @brief Package dependency
 */
struct di_package_dependency
{
  di_package *ptr;                                      /**< the package, may be NULL */
};

/**
 * @brief Group of package dependencies
 */
struct di_package_dependency_group
{
  di_package_dependency_type type;                      /**< type of dependency */
  di_slist list;                                        /**< includes di_package_dependency */
};

/**
 * @brief Package description
 */
struct di_package_description
{
  char *language;                                       /**< language for description, "" for default */
  char *short_description;                              /**< description */
  char *description;                                    /**< description */
};

/**
 * @brief Packages file
 */
struct di_packages
{
  di_hash_table *table;                                 /**< includes di_package */
  di_slist list;                                        /**< includes di_package */
  unsigned int resolver;                                /**< @internal */
};

/**
 * @internal
 * @brief Packages file - Allocator
 */
struct di_packages_allocator
{
  di_mem_chunk *package_mem_chunk;                      /**< @internal */
  di_mem_chunk *package_dependency_mem_chunk;           /**< @internal */
  di_mem_chunk *package_dependency_group_mem_chunk;     /**< @internal */
  di_mem_chunk *package_description_mem_chunk;          /**< @internal */
  di_mem_chunk *slist_node_mem_chunk;                   /**< @internal */
};

di_packages *di_packages_alloc (void);
void di_packages_free (di_packages *packages);

di_packages_allocator *di_packages_allocator_alloc (void);
void di_packages_allocator_free (di_packages_allocator *packages);

/**
 * @}
 * @addtogroup di_packages_parser
 * @{
 */

struct di_packages_parser_data
{
  union
  {
    di_package *package;
    di_packages *packages;
  };
  di_packages_allocator *allocator;
};

di_packages *di_packages_read_file (const char *file, di_packages_allocator *allocator);
di_packages *di_packages_read_file_special (const char *file, di_packages_allocator *allocator, di_parser_info *info);
di_package *di_packages_control_read_file (const char *file, di_packages_allocator *allocator);
di_packages *di_packages_status_read_file (const char *file, di_packages_allocator *allocator);
int di_packages_write_file (di_packages *packages, const char *file);
int di_packages_status_write_file (di_packages *packages, const char *file);

/**
 * @}
 * @addtogroup di_packages
 * @{
 */

di_package *di_packages_get_package (di_packages *packages, const char *name, size_t n);
di_package *di_packages_get_package_new (di_packages *packages, di_packages_allocator *allocator, char *name, size_t n);

di_package_dependency *di_package_dependency_alloc (di_packages_allocator *allocator);
di_package_dependency_group *di_package_dependency_group_alloc (di_packages_allocator *allocator);
di_package_description *di_package_description_alloc (di_packages_allocator *allocator);

di_slist *di_packages_resolve_dependencies (di_packages *packages, di_slist *list, di_packages_allocator *allocator);

/** @} */

di_parser_fields_function_read
  di_packages_parser_read_dependency,
  di_packages_parser_read_description,
  di_packages_parser_read_name,
  di_packages_parser_read_priority,
  di_packages_parser_read_status;

di_parser_fields_function_write
  di_packages_parser_write_dependency,
  di_packages_parser_write_description,
  di_packages_parser_write_priority,
  di_packages_parser_write_status;

/**
 * @addtogroup di_packages_parser
 * @{
 */

extern const di_parser_fieldinfo *di_packages_parser_fieldinfo[];
extern const di_parser_fieldinfo *di_packages_status_parser_fieldinfo[];
extern const di_parser_fieldinfo *di_packages_minimal_parser_fieldinfo[];

/** @} */
#endif
