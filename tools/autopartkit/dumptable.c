#include <stdarg.h>
#include <stdio.h>
#include "autopartkit.h"

void
autopartkit_error(int fatal, const char * format, ...)
{
    va_list ap;

    va_start(ap, format);
    fprintf(stderr, "error: ");
    vfprintf(stderr, format, ap);
    va_end(ap);
}

void autopartkit_log(const int level, const char * format, ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

void
list_dump(diskspace_req_t *list)
{
  int total = 0;
  float percent_total = 0.0;
  int i;
  if (!list)
  {
    printf("No list!\n");
    return;
  }
  printf("Dumping diskspace_request list:\n");
  for (i=0; list && list[i].mountpoint; i++)
  {
    printf(" %s %s %d %d %f\n", list[i].mountpoint, list[i].fstype,
	   list[i].minsize, list[i].maxsize, list[i].percent_total);
    total += list[i].minsize;
    percent_total += list[i].percent_total;
  }
  printf(" total %d - %f\n", total, percent_total);
}

int
main(int argc, char *argv[])
{
  diskspace_req_t *list;
  char *infile;
  if (2 == argc)
    infile = argv[1];
  else
    infile = "workstation.table";

  list = load_partitions(infile);
  list_dump(list);
  free_partition_list(list);

  return 0;
}

