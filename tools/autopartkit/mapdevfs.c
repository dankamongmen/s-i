/*
 *  map devfs names to old-style names
 *
 */

#define _GNU_SOURCE /* Make asprintf available */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "autopartkit.h"

/* copied from libdebian-installer, there it is named di_mapdevfs() */
ssize_t
mapdevfs(const char *path, char *buf, size_t n)
{
  static struct entry
  {
    unsigned char major;
    unsigned char minor;
    char *name;
    enum { ENTRY_TYPE_ONE, ENTRY_TYPE_NUMBER, ENTRY_TYPE_DISC } type;
    unsigned char entry_first;
    unsigned char entry_disc_minor_shift;
  }
  entries[] =
  {
    /*
       major	minor	name		type           entry_first entry_disc_minor_shift */
    { 2,	0,	"fd",		ENTRY_TYPE_NUMBER,	0,	0 },
    { 3,	0,	"hd",		ENTRY_TYPE_DISC,	0,	6 },
    { 4,	64,	"ttyS",		ENTRY_TYPE_NUMBER,	0,	6 },
    { 4,	0,	"tty",		ENTRY_TYPE_NUMBER,	0,	6 },
    { 8,	0,	"sd",		ENTRY_TYPE_DISC,	0,	4 },
    { 9,	0,      "md",           ENTRY_TYPE_NUMBER,      0,      0 },
    { 22,	0,	"hd",		ENTRY_TYPE_DISC,	2,	6 },
    { 33,	0,	"hd",		ENTRY_TYPE_DISC,	4,	6 },
    { 34,	0,	"hd",		ENTRY_TYPE_DISC,	6,	6 },
    { 56,	0,	"hd",		ENTRY_TYPE_DISC,	8,	6 },
    { 57,	0,	"hd",		ENTRY_TYPE_DISC,	10,	6 },
    { 65,	0,	"sd",		ENTRY_TYPE_DISC,	16,	4 },
    { 66,	0,	"sd",		ENTRY_TYPE_DISC,	32,	4 },
    { 67,	0,	"sd",		ENTRY_TYPE_DISC,	48,	4 },
    { 68,	0,	"sd",		ENTRY_TYPE_DISC,	64,	4 },
    { 69,	0,	"sd",		ENTRY_TYPE_DISC,	80,	4 },
    { 70,	0,	"sd",		ENTRY_TYPE_DISC,	96,	4 },
    { 71,	0,	"sd",		ENTRY_TYPE_DISC,	112,	4 },
    { 88,	0,	"hd",		ENTRY_TYPE_DISC,	12,	6 },
    { 89,	0,	"hd",		ENTRY_TYPE_DISC,	14,	6 },
    { 90,	0,	"hd",		ENTRY_TYPE_DISC,	16,	6 },
    { 91,	0,	"hd",		ENTRY_TYPE_DISC,	18,	6 },
    { 94,	0,	"dasd",		ENTRY_TYPE_DISC,	0,	2 },
    { 0,	0,	NULL,		ENTRY_TYPE_ONE,		0,	0 },
  };

  struct stat s;
  struct entry *e;

  ssize_t ret = 0;

  unsigned int unit;
  unsigned int part;

  if (!strcmp("none", path))
    return snprintf(buf, n, "%s", path);

  if (stat(path,&s) == -1)
    return 0;

  e = entries;
  while (e->name != NULL) {
    if (major(s.st_rdev) == e->major && 
        ((e->type == ENTRY_TYPE_ONE && minor(s.st_rdev) == e->minor) ||
         (e->type != ENTRY_TYPE_ONE && minor(s.st_rdev) >= e->minor))) {
      break;
    }
    e++;
  }
  if ( ! e->name )
    /* Pass unknown devices on without changes.  This fixes LVM devices */
    return snprintf(buf, n, "%s", path);

  switch (e->type)
  {
    case ENTRY_TYPE_ONE:
      ret = snprintf(buf, n, "/dev/%s", e->name);
      break;

    case ENTRY_TYPE_NUMBER:
      unit = minor(s.st_rdev) - e->minor + e->entry_first;

      ret = snprintf(buf, n, "/dev/%s%d", e->name, unit);
      break;

    case ENTRY_TYPE_DISC:
      unit = (minor(s.st_rdev) >> e->entry_disc_minor_shift);
      part = (minor(s.st_rdev) & ((1 << e->entry_disc_minor_shift) -1 ));

      unit += e->entry_first;

      if (unit+'a' > 'z') {
        unit -= 26;
        ret = snprintf(buf, n, "/dev/%s%c%c", e->name, 'a' + unit / 26, 'a' + unit % 26);
      }
      else
        ret = snprintf(buf, n, "/dev/%s%c", e->name, 'a' + unit);

      if (part)
        ret = snprintf(buf + ret, n - ret, "%d", part);

      break;
  };

  return ret;
}

/* Returns the devfs path name normalized into a "normal" (hdaX, sdaX)
 * name.  The return value is malloced, which means that the caller
 * needs to free it.  This is kinda hairy, but I don't see any other
 * way of operating with non-devfs systems when we use devfs on the
 * boot medium.  The numbers are taken from devices.txt in the
 * Documentation subdirectory off the kernel sources.
 */

char *normalize_devfs(const char* path)
{
    char buf[1024];
    if (0 == mapdevfs(path, buf, sizeof(buf)))
      return NULL;
    return strdup(buf);
}

#if AS_PROGRAM
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Wrong number of args: mapdevfs <path>\n");
        return 1;
    }
    printf("%s\n", normalize_devfs(argv[1]));
    return 0;
}
#endif
