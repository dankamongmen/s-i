/*
 * package.c
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
 * $Id: package.c,v 1.3 2003/09/30 19:22:07 waldi Exp $
 */

#include <debian-installer/package_internal.h>

#include <debian-installer/packages_internal.h>

#include <ctype.h>

void internal_di_package_destroy_func (void *data)
{
  di_package_destroy (data);
}

void di_package_destroy (di_package *package)
{
  di_free (package->package);
  di_free (package->section);
  di_free (package->maintainer);
  di_free (package->architecture);
  di_free (package->version);
  di_free (package->filename);
  di_free (package->md5sum);
  di_free (package->short_description);
  di_free (package->description);

  memset (package, 0, sizeof (di_package));
}

void di_package_version_free (di_package_version *version)
{
  di_free (version);
}

#define order(x) ((x) == '~' ? -1 \
        : isdigit((x)) ? 0 \
        : !(x) ? 0 \
        : isalpha((x)) ? (x) \
        : (x) + 256)

static int verrevcmp (const char *val, const char *ref)
{
  if (!val)
    val = "";
  if (!ref)
    ref = "";

  while (*val || *ref) {
    int first_diff = 0;

    while ((*val && !isdigit (*val)) || (*ref && !isdigit (*ref))) {
      int vc = order (*val), rc = order (*ref);
      if (vc != rc)
        return vc - rc;
      val++;
      ref++;
    }

    while (*val == '0')
      val++;
    while (*ref == '0')
      ref++;
    while (isdigit (*val) && isdigit (*ref)) {
      if (!first_diff)
        first_diff = *val - *ref;
      val++;
      ref++;
    }
    if (isdigit (*val))
      return 1;
    if (isdigit (*ref))
      return -1;
    if (first_diff)
      return first_diff;
  }
  return 0;
}

int di_package_version_compare(const di_package_version *a, const di_package_version *b)
{
  int r;

  if (a->epoch > b->epoch)
    return 1;
  if (a->epoch < b->epoch)
    return -1;
  r = verrevcmp(a->upstream, b->upstream);
  if (r != 0)
    return r;
  return verrevcmp(a->debian_revision, b->debian_revision);
}

di_package_version *di_package_version_parse (di_package *package)
{
  di_package_version *version;
  char *hyphen, *colon, *eepochcolon, *string;
  const char *end;
  unsigned long epoch;

  if (!package->version)
    return NULL;

  version = di_new0 (di_package_version, 1);

  string = package->version;
  end = string + strlen (string);
  colon = strchr (string, ':');
  if (colon) {
    epoch = strtoul (package->version, &eepochcolon, 10);
    if (colon != eepochcolon)
      return 0;
    if (!*++colon)
      return 0;
    string = colon;
    version->epoch = epoch;
  }
  hyphen = strrchr (string, '-');
  if (hyphen)
  {
    version->upstream = di_stradup (string, hyphen - string);
    version->debian_revision = di_stradup (hyphen + 1, end - hyphen - 1);
  }
  else
    version->upstream = di_stradup (string, end - string);

  return version;
}

