/*
 * devfs.c
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
 * $Id$
 */

#include <debian-installer/system/devfs.h>

#include <debian-installer/string.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* Returns the devfs path name normalized into a "normal" (hdaX, sdaX)
 * name.  The return value is malloced, which means that the caller
 * needs to free it.  This is kinda hairy, but I don't see any other
 * way of operating with non-devfs systems when we use devfs on the
 * boot medium.  The numbers are taken from devices.txt in the
 * Documentation subdirectory off the kernel sources.
 */

ssize_t di_system_devfs_map_from (const char *path, char *buf, size_t n)
{
  static struct entry
  {
    unsigned char major;
    unsigned char minor;
    char *name;
    enum { ENTRY_TYPE_ONE, ENTRY_TYPE_NUMBER, ENTRY_TYPE_DISC, ENTRY_TYPE_DISC_ARRAY } type;
    unsigned char entry_first;
    unsigned char entry_disc_minor_shift;
    enum { FORMAT_DEFAULT, FORMAT_NOCONTROLLER } format;
  }
  entries[] =
  {
    /*
      bmajor	minor	name		type			entry_first
      									entry_disc_minor_shift
										format
    */
    { 2,	0,	"fd",		ENTRY_TYPE_NUMBER,	0,	0,	FORMAT_DEFAULT },
    { 3,	0,	"hd",		ENTRY_TYPE_DISC,	0,	6,	FORMAT_DEFAULT },
    { 4,	64,	"ttyS",		ENTRY_TYPE_NUMBER,	0,	0,	FORMAT_DEFAULT },
    { 4,	0,	"tty",		ENTRY_TYPE_NUMBER,	0,	0,	FORMAT_DEFAULT },
    { 8,	0,	"sd",		ENTRY_TYPE_DISC,	0,	4,	FORMAT_DEFAULT },
    { 9,	0,	"md",		ENTRY_TYPE_NUMBER,	0,	0,	FORMAT_DEFAULT },
    { 11,	0,	"scd",		ENTRY_TYPE_NUMBER,	0,	0,	FORMAT_DEFAULT },
    { 22,	0,	"hd",		ENTRY_TYPE_DISC,	2,	6,	FORMAT_DEFAULT },
    { 33,	0,	"hd",		ENTRY_TYPE_DISC,	4,	6,	FORMAT_DEFAULT },
    { 34,	0,	"hd",		ENTRY_TYPE_DISC,	6,	6,	FORMAT_DEFAULT },
    { 48,	0,	"rd",		ENTRY_TYPE_DISC_ARRAY,	0,	3,	FORMAT_DEFAULT },
    { 49,	0,	"rd",		ENTRY_TYPE_DISC_ARRAY,	1,	3,	FORMAT_DEFAULT },
    { 50,	0,	"rd",		ENTRY_TYPE_DISC_ARRAY,	2,	3,	FORMAT_DEFAULT },
    { 51,	0,	"rd",		ENTRY_TYPE_DISC_ARRAY,	3,	3,	FORMAT_DEFAULT },
    { 52,	0,	"rd",		ENTRY_TYPE_DISC_ARRAY,	4,	3,	FORMAT_DEFAULT },
    { 53,	0,	"rd",		ENTRY_TYPE_DISC_ARRAY,	5,	3,	FORMAT_DEFAULT },
    { 54,	0,	"rd",		ENTRY_TYPE_DISC_ARRAY,	6,	3,	FORMAT_DEFAULT },
    { 55,	0,	"rd",		ENTRY_TYPE_DISC_ARRAY,	7,	3,	FORMAT_DEFAULT },
    { 56,	0,	"hd",		ENTRY_TYPE_DISC,	8,	6,	FORMAT_DEFAULT },
    { 57,	0,	"hd",		ENTRY_TYPE_DISC,	10,	6,	FORMAT_DEFAULT },
    { 65,	0,	"sd",		ENTRY_TYPE_DISC,	16,	4,	FORMAT_DEFAULT },
    { 66,	0,	"sd",		ENTRY_TYPE_DISC,	32,	4,	FORMAT_DEFAULT },
    { 67,	0,	"sd",		ENTRY_TYPE_DISC,	48,	4,	FORMAT_DEFAULT },
    { 68,	0,	"sd",		ENTRY_TYPE_DISC,	64,	4,	FORMAT_DEFAULT },
    { 69,	0,	"sd",		ENTRY_TYPE_DISC,	80,	4,	FORMAT_DEFAULT },
    { 70,	0,	"sd",		ENTRY_TYPE_DISC,	96,	4,	FORMAT_DEFAULT },
    { 71,	0,	"sd",		ENTRY_TYPE_DISC,	112,	4,	FORMAT_DEFAULT },
    { 72,	0,	"ida",		ENTRY_TYPE_DISC_ARRAY,	0,	4,	FORMAT_DEFAULT },
    { 73,	0,	"ida",		ENTRY_TYPE_DISC_ARRAY,	1,	4,	FORMAT_DEFAULT },
    { 74,	0,	"ida",		ENTRY_TYPE_DISC_ARRAY,	2,	4,	FORMAT_DEFAULT },
    { 75,	0,	"ida",		ENTRY_TYPE_DISC_ARRAY,	3,	4,	FORMAT_DEFAULT },
    { 76,	0,	"ida",		ENTRY_TYPE_DISC_ARRAY,	4,	4,	FORMAT_DEFAULT },
    { 77,	0,	"ida",		ENTRY_TYPE_DISC_ARRAY,	5,	4,	FORMAT_DEFAULT },
    { 78,	0,	"ida",		ENTRY_TYPE_DISC_ARRAY,	6,	4,	FORMAT_DEFAULT },
    { 79,	0,	"ida",		ENTRY_TYPE_DISC_ARRAY,	7,	4,	FORMAT_DEFAULT },
    { 88,	0,	"hd",		ENTRY_TYPE_DISC,	12,	6,	FORMAT_DEFAULT },
    { 89,	0,	"hd",		ENTRY_TYPE_DISC,	14,	6,	FORMAT_DEFAULT },
    { 90,	0,	"hd",		ENTRY_TYPE_DISC,	16,	6,	FORMAT_DEFAULT },
    { 91,	0,	"hd",		ENTRY_TYPE_DISC,	18,	6,	FORMAT_DEFAULT },
    { 94,	0,	"dasd",		ENTRY_TYPE_DISC,	0,	2,	FORMAT_DEFAULT },
    { 104,	0,	"cciss",	ENTRY_TYPE_DISC_ARRAY,	0,	4,	FORMAT_DEFAULT },
    { 105,	0,	"cciss",	ENTRY_TYPE_DISC_ARRAY,	1,	4,	FORMAT_DEFAULT },
    { 106,	0,	"cciss",	ENTRY_TYPE_DISC_ARRAY,	2,	4,	FORMAT_DEFAULT },
    { 107,	0,	"cciss",	ENTRY_TYPE_DISC_ARRAY,	3,	4,	FORMAT_DEFAULT },
    { 108,	0,	"cciss",	ENTRY_TYPE_DISC_ARRAY,	4,	4,	FORMAT_DEFAULT },
    { 109,	0,	"cciss",	ENTRY_TYPE_DISC_ARRAY,	5,	4,	FORMAT_DEFAULT },
    { 110,	0,	"cciss",	ENTRY_TYPE_DISC_ARRAY,	6,	4,	FORMAT_DEFAULT },
    { 111,	0,	"cciss",	ENTRY_TYPE_DISC_ARRAY,	7,	4,	FORMAT_DEFAULT },
    { 114,	0,	"ataraid",	ENTRY_TYPE_DISC_ARRAY,	0,	4,	FORMAT_NOCONTROLLER },
    { 0,	0,	NULL,		ENTRY_TYPE_ONE,		0,	0,	FORMAT_DEFAULT },
  };

  struct stat s;
  struct entry *e;

  ssize_t ret = 0;

  unsigned int disc;
  unsigned int part;

  if (!strcmp ("none", path))
    return snprintf (buf, n, "%s", path);

  if (stat (path,&s) == -1)
    return 0;

  e = entries;
  while (e->name != NULL) {
    if (major (s.st_rdev) == e->major && 
        ((e->type == ENTRY_TYPE_ONE && minor (s.st_rdev) == e->minor) ||
         (e->type != ENTRY_TYPE_ONE && minor (s.st_rdev) >= e->minor))) {
      break;
    }
    e++;
  }
  if (!e->name || !S_ISBLK(s.st_mode)) {
#ifdef TEST
    fprintf(stderr, "(unknown device)\n");
#endif
    /* Pass unknown devices on without changes. */
    return snprintf (buf, n, "%s", path);
  }
  
  strcat (buf, "/dev/");

  switch (e->type)
  {
    case ENTRY_TYPE_ONE:
      ret = di_snprintfcat (buf, n, "%s", e->name);
      break;

    case ENTRY_TYPE_NUMBER:
      disc = minor (s.st_rdev) - e->minor + e->entry_first;

      ret = di_snprintfcat (buf, n, "%s%d", e->name, disc);
      break;

    case ENTRY_TYPE_DISC:
    case ENTRY_TYPE_DISC_ARRAY:
      disc = (minor (s.st_rdev) >> e->entry_disc_minor_shift);
      part = (minor (s.st_rdev) & ((1 << e->entry_disc_minor_shift) - 1));

      switch (e->type)
      {
        case ENTRY_TYPE_DISC:
          disc += e->entry_first;

          if (disc + 'a' > 'z')
          {
            disc -= 26;
            ret = di_snprintfcat (buf, n, "%s%c%c", e->name, 'a' + disc / 26, 'a' + disc % 26);
          }
          else
            ret = di_snprintfcat (buf, n, "%s%c", e->name, 'a' + disc);

          if (part)
            ret = di_snprintfcat (buf, n, "%d", part);

          break;
        case ENTRY_TYPE_DISC_ARRAY:
	  if (e->format == FORMAT_DEFAULT)
	          ret = di_snprintfcat (buf, n, "%s/c%dd%d", e->name, e->entry_first, disc);
	  else if (e->format == FORMAT_NOCONTROLLER)
	          ret = di_snprintfcat (buf, n, "%s/d%d", e->name, disc);

          if (part)
            ret = di_snprintfcat (buf, n, "p%d", part);

          break;
        default:
          break;
      }
      break;
  };
  
  return ret;
}

#ifndef TEST
ssize_t di_mapdevfs (const char *path, char *buf, size_t n) __attribute__ ((alias("di_system_devfs_map_from")));
#else
/* Build standalone binary with -DTEST */
main (int argc, char **argv) {
  static char ret[256];
  if (argc != 2) {
    fprintf(stderr, "wrong number of args\n");
    exit(1);
  }
  di_system_devfs_map_from(argv[1], ret, sizeof(ret));
  printf("%s => %s", argv[1], ret);
  if (strcmp(argv[1], ret) == 0) {
    printf("\t\t(SAME)");
  }
  printf("\n");
  exit(0);
}
#endif
