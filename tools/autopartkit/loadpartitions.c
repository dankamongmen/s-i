/*
 * Load the partition specification from disk.
 *
 * Author: Petter Reinholdtsen <pere@hungry.com>
 * Date:   2002-04-14
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "autopartkit.h"

struct partition_list {
  diskspace_req_t *disk_reqs;
  unsigned int capacity;
  unsigned int count;
};

static int
list_makeroom(struct partition_list *list, unsigned int room)
{
  unsigned int newsize;
  void *mem;
  assert(list);
  if (list->capacity > list->count + room)
  {
    autopartkit_log(4, "Enough room in list %d > %d + %d\n",
	            list->capacity, list->count, room);
    return 0;
  }

  newsize = list->capacity * 2 ;
  if (room > newsize)
    newsize = room;

  mem = realloc(list->disk_reqs, newsize * sizeof(list->disk_reqs[0]));
  if (NULL == mem)
    return -1;
  else
  {
    list->capacity = newsize;
    list->disk_reqs = mem;
    return 0;
  }
}

static int
list_add_entry(struct partition_list *list, char *mountpoint, 
	       char *fstype, int minsize, int maxsize, int ondisk)
{
  if (0 != list_makeroom(list, 1))
    return -1;

  if (0 != mountpoint)
    mountpoint = strdup(mountpoint);

  if (0 != fstype)
    fstype = strdup(fstype);

  list->disk_reqs[list->count].mountpoint = mountpoint;
  list->disk_reqs[list->count].fstype = fstype;
  list->disk_reqs[list->count].minsize = minsize;
  list->disk_reqs[list->count].maxsize = maxsize;
  list->disk_reqs[list->count].ondisk = ondisk;
  (list->count)++;

  return 0;
}

static int
list_init(struct partition_list *list)
{
  assert(list);
  list->disk_reqs = NULL;
  list->capacity = 0;
  list->count = 0;
  return 0;
}

static void
remove_junk(char *buf, unsigned int buflength)
{
  /* Terminate on comment char */
  unsigned int i;
  for (i=0; i < buflength; ++i)
    if ('#' == buf[i] || '\n' == buf[i])
      buf[i] = '\0';
}

static int
add_partition(struct partition_list *list, char *line)
{
  char mountpoint[1024];
  char fstype[1024];
  int minsize;
  int maxsize;
  int ondisk;

  if (!line || ! *line) /* Ignore empty lines */
    return -1;

  assert(list);

  autopartkit_log(3, "Adding '%s'\n", line);

  if (4 != sscanf(line, "%s %s %d %d ", mountpoint, fstype, &minsize,
		  &maxsize))
  {
      autopartkit_log(3, "Failed to parse line.\n");
      return -1; /* error */
  }

  autopartkit_log(2, "Fetched partition info %s %s %d %d\n",
                  mountpoint, fstype, minsize, maxsize);
  if (maxsize != 0)
      ondisk = 1;
  else
      ondisk = 0;

  return list_add_entry(list, mountpoint, fstype, minsize, maxsize, ondisk);
}

diskspace_req_t *
load_partitions(const char *filename)
{
  struct partition_list list;
  FILE *fp;
  char buf[1024];

  memset(buf, 0, sizeof(buf));

  list_init(&list);

  fp = fopen(filename, "r");
  if (NULL == fp)
  {
    autopartkit_error(0, "Failed to open table file '%s'\n", filename);
    return NULL;
  }
  autopartkit_log(2, "Loading table file '%s'\n", filename);
  while ( ! feof(fp) )
  {
    if (NULL != fgets(buf, sizeof(buf), fp))
    {
      remove_junk(buf, sizeof(buf));
      add_partition(&list, buf);
    }
  }

  fclose(fp);

  /* Terminate list */
  list_add_entry(&list, NULL, NULL, -1, -1, 0);

  return list.disk_reqs; /* Transfering resposibility for this memory block */
}

void
free_partition_list(diskspace_req_t *list)
{
  int i;
  for (i=0; list && list[i].mountpoint; i++)
  {
    if (list[i].mountpoint)
      free(list[i].mountpoint);
    if (list[i].fstype)
      free(list[i].fstype);
  }
  free(list);
}
