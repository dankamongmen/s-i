/*
 * parser.c
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
 * $Id: parser.c,v 1.2 2003/09/29 12:10:00 waldi Exp $
 */

#include <debian-installer/parser.h>

/**
 * @return a di_parser_info
 */
di_parser_info *di_parser_info_alloc (void)
{
  di_parser_info *info;

  info = di_new0 (di_parser_info, 1);
  info->table = di_hash_table_new (di_rstring_hash, di_rstring_equal);

  return info;
}

/**
 * @param info info to free
 */
void di_parser_info_free (di_parser_info *info)
{
  di_hash_table_destroy (info->table);
  di_free (info);
}

void di_parser_info_add (di_parser_info *info, const di_parser_fieldinfo *fieldinfo[])
{
  di_parser_fieldinfo **fip;

  for (fip = (di_parser_fieldinfo **) fieldinfo; *fip; fip++)
  {
    di_hash_table_insert (info->table, &(*fip)->key, *fip);
    di_slist_append (&info->list, *fip);
  }
}

void di_parser_read_boolean (data, fip, field_modifier, value, user_data)
  void **data;
  const di_parser_fieldinfo *fip __attribute__ ((unused));
  di_rstring *field_modifier __attribute__ ((unused));
  di_rstring *value;
  void *user_data __attribute__ ((unused));
{
  int *f = (int *)((char *)*data + fip->integer);
  if (strncasecmp (value->string, "yes", 3) == 0)
    *f = 1;
  else
    *f = 0;
}

void di_parser_write_boolean (data, fip, callback, callback_data, user_data)
  void **data;
  const di_parser_fieldinfo *fip;
  di_parser_fields_function_write_callback callback;
  void *callback_data;
  void *user_data __attribute__ ((unused));
{
  int *f = (int *)((char *)*data + fip->integer);
  di_rstring value = { "Yes", 3 };

  if (*f)
    callback (&fip->key, &value, callback_data);
}

void di_parser_read_string (data, fip, field_modifier, value, user_data)
  void **data;
  const di_parser_fieldinfo *fip __attribute__ ((unused));
  di_rstring *field_modifier __attribute__ ((unused));
  di_rstring *value;
  void *user_data __attribute__ ((unused));
{
  char **f = (char **)((char *)*data + fip->integer);
  *f = di_stradup (value->string, value->size);
}

void di_parser_write_string (data, fip, callback, callback_data, user_data)
  void **data;
  const di_parser_fieldinfo *fip;
  di_parser_fields_function_write_callback callback;
  void *callback_data;
  void *user_data __attribute__ ((unused));
{
  char **f = (char **)((char *)*data + fip->integer);
  di_rstring value;

  if (*f)
  {
    value.string = *f;
    value.size = strlen (*f);
    callback (&fip->key, &value, callback_data);
  }
}

void di_parser_read_int (data, fip, field_modifier, value, user_data)
  void **data;
  const di_parser_fieldinfo *fip __attribute__ ((unused));
  di_rstring *field_modifier __attribute__ ((unused));
  di_rstring *value;
  void *user_data __attribute__ ((unused));
{
  int *f = (int *)((char *)*data + fip->integer);
  *f = strtol (value->string, NULL, 10);
}

void di_parser_write_int (data, fip, callback, callback_data, user_data)
  void **data;
  const di_parser_fieldinfo *fip;
  di_parser_fields_function_write_callback callback;
  void *callback_data;
  void *user_data __attribute__ ((unused));
{
  int *f = (int *)((char *)*data + fip->integer);
  char value_buf[32];
  di_rstring value = { value_buf, 0 };

  if (*f)
  {
    value.size = snprintf (value_buf, 32, "%d", *f);
    callback (&fip->key, &value, callback_data);
  }
}


