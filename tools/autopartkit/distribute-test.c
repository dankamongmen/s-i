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
#include <string.h>
#include "autopartkit.h"
#include <parted/parted.h>
#include <assert.h>

#include "parted-compat.h"

#define EMPTYGEOMETRY {NULL, 0, 0, 0}

struct disk_info_t diskinfo[] = {
    { "/dev/hda", EMPTYGEOMETRY,   MiB_TO_BLOCKS( 400), MiB_TO_BLOCKS( 400) },
    { "/dev/hdb", EMPTYGEOMETRY,   MiB_TO_BLOCKS(1000), MiB_TO_BLOCKS(1000) },
    { "/dev/hdc", EMPTYGEOMETRY,   MiB_TO_BLOCKS(2000), MiB_TO_BLOCKS(2000) },
    { NULL,       EMPTYGEOMETRY,   MiB_TO_BLOCKS(   0), MiB_TO_BLOCKS(   0) }
};

static int
cmp_spaceinfo_path(const void *ap, const void *bp)
{
    const struct disk_info_t *a = (const struct disk_info_t *)ap;
    const struct disk_info_t *b = (const struct disk_info_t *)bp;
    return strcmp(a->path, b->path);
}

int
main(int argc, char *argv[])
{
    diskspace_req_t *reqs;
    int retval;
    int partnum;
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

    /* Do not make LVM logical volumes on the disk */
    for (partnum = 0; partnum < 10 /* MAX_PARTITIONS */
	   && reqs[partnum].fstype;
	 ++partnum)
        if ( 0 == strncmp("lvm:", reqs[partnum].fstype, 4) )
	    reqs[partnum].ondisk = 0;

    retval = distribute_partitions(spaceinfo, reqs);

    if (retval)
        autopartkit_log(0, "partitioning failed.\n");
    else
        print_list(spaceinfo, reqs);

    free_partition_list(reqs);

    if (diskinfo != spaceinfo)
        free(spaceinfo);

    PED_DONE();

    return 0;
}
