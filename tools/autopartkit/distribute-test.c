/* 

   distribute-test.c - Part of autopartkit, a module to partition
                       devices for debian-installer.

   Author - Petter Reinholdtsen

   Copyright (C) 2002  Petter Reinholdtsen <pere@hungry.com>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "autopartkit.h"
#include <parted/parted.h>
#include <assert.h>

#include "parted-compat.h"

#define EMPTYGEOMETRY {NULL, 0, 0, 0}

struct disk_info_t diskinfo[] = {
    { "/dev/hda", EMPTYGEOMETRY,   MiB_TO_BLOCKS( 400), MiB_TO_BLOCKS( 400) },
    { "/dev/hdb", EMPTYGEOMETRY,   MiB_TO_BLOCKS(1000), MiB_TO_BLOCKS(1000) },
    { "/dev/hdc", EMPTYGEOMETRY,   MiB_TO_BLOCKS(2000), MiB_TO_BLOCKS(2000) },
    { NULL, EMPTYGEOMETRY, 0, 0 }
};

void
autopartkit_error(int fatal, const char * format, ...)
{
    va_list ap;

    va_start(ap, format);
    fprintf(stderr, "error: ");
    vfprintf(stderr, format, ap);
    va_end(ap);
    if (fatal)
      exit(fatal);
}

void
autopartkit_log(const int level, const char * format, ...)
{
    va_list ap;

    fprintf(stderr, "log %d: ", level);

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

int
main(int argc, char *argv[])
{
    diskspace_req_t *reqs;  
    int retval;
    char *infile;
    struct disk_info_t *spaceinfo = NULL;

    PED_INIT();

    if (2 == argc)
        infile = argv[1];
    else
        infile = "default.table";

    reqs = load_partitions(infile);

    if (NULL == reqs)
    {
        PED_DONE();
        return 1;
    }

    spaceinfo = get_free_space_list();

    if (NULL == spaceinfo)
      {
        autopartkit_log(0, "no free space list, using hardcoded table.\n");
	spaceinfo = diskinfo;
      }

    retval = distribute_partitions(spaceinfo, reqs);

    if (retval)
        autopartkit_log(0, "partitioning failed.\n");
    else
        print_list(spaceinfo, reqs);

    free_partition_list(reqs);

    PED_DONE();

    return 0;
}
