/*

   distribute.c - Part of autopartkit, a module to partition devices
                  for debian-installer.

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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


 The partition distribution algorithm:

 1. For all requested partitions, place the partition on the first
    disk with enough free space to fullfill the minimum size
    requirement.  Reduce the amount of free space available on the
    disk for each partition added to the disk.

 2. Do the following for each disk:

   1. Calculate the sum of all additionally requested diskspace
      (max-min).  Each disk with no limit to the maximum size
      get maximum equal to the disk capacity to make sure this
      partition will get the whole disk if there are no other
      partitions requesting more diskspace on this disk, and that
      the other partitions will get a share of the disk.  The other
      partitions would not get any share if unlimited request was
      treated as an infinite number.

   2. For each partition on the disk, calculate the new size by first
      calculating how much more diskspace is requested for the given
      partition (max-min), and then multiplying this size with the
      free space on the disk divided by the sum of all requested
      extra space.  This will give how much of the available free
      space this partition can get if there is less free space then
      requested space.  If there is more free space than requested
      size, the requested extra space will be bigger then the
      maximum size, and must be truncated.  The new size is set to
      the current size plus the allocated extra space, and the
      amount of free space on the disk is reduced by the extra space
      allocated.  The sum of requested diskspace for this disk is
      reduced by (max-min) to make sure the free space number is
      comparable to the requested diskspace.

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "autopartkit.h"
#include <assert.h>

#include "parted-compat.h"

#ifdef __linux__
#include <fcntl.h>
#include <sys/ioctl.h>

/* from <linux/cdrom.h> */
#define CDROM_GET_CAPABILITY	0x5331	/* get capabilities */
#endif /* __linux__ */

#define MAX(a,b) ((a) > (b) ? (a) : (b))

/*
 * Find and return the first disk with size amount of free space.
 */
static struct disk_info_t *
find_disk_with_freespace(struct disk_info_t diskinfo[], PedSector blocks)
{
    int i;
    for (i = 0; diskinfo[i].capacity; i++)
    {
	if (diskinfo[i].freespace >= blocks)
	    return &diskinfo[i];
    }
    return NULL;
}

/*
 * Decide how to distribute the requested partitions on the available
 * disks.
 */
int
distribute_partitions(struct disk_info_t diskinfo[],
		      struct diskspace_req_s reqs[])
{
    int i;
    PedSector maxmax_blk = 0;

    /* Initialize reqs and convert MiB to blocks. */
    for (i = 0; reqs[i].mountpoint; ++i)
    {
	reqs[i].min_blk = MiB_TO_BLOCKS(reqs[i].minsize);
	if (-1 == reqs[i].maxsize)
	    reqs[i].max_blk = (PedSector)-1;
	else
	    reqs[i].max_blk = MiB_TO_BLOCKS(reqs[i].maxsize);
	reqs[i].blocks  = 0;
	reqs[i].curdisk = NULL;
    }

    /* Distribute the partiontions using minimum sizes */
    for (i = 0; reqs[i].mountpoint; ++i)
    {
	struct disk_info_t *disk;
	if (! reqs[i].ondisk)
	    continue; /* Ignore mount points without disk partition */
	disk = find_disk_with_freespace(diskinfo, reqs[i].min_blk);
	if (NULL == disk)
	{
	    /* Unable to find disk with enough free space */
	    autopartkit_error(0, "Unable to find %llu blocks of "
			      "free space requested by %s on any disk.\n",
			      reqs[i].min_blk,
			      reqs[i].mountpoint);
	    return -1;
	}
	else
	{
            maxmax_blk = MAX(maxmax_blk, reqs[i].max_blk);
            maxmax_blk = MAX(maxmax_blk, disk->capacity);

	    /* Found a usable disk.  Add this partition to the disk. */
	    reqs[i].blocks  = reqs[i].min_blk;
	    reqs[i].curdisk = disk;
	    disk->freespace -= reqs[i].min_blk;
	}
    }

    /* Grow the partitions as much as possible on the allocated disk. */
    for (i = 0; diskinfo[i].capacity; i++)
    {
	int j;
	int64_t total_wanted = 0;
	/* First find out how much extra space the partitions on this disk
	   want, ie the sum of all max_blk-min_blk */
	for (j = 0; reqs[j].mountpoint; ++j)
	{
	    if (0 == reqs[j].max_blk)
	        continue; /* Ignore zero-size partitions */
	    if (reqs[j].curdisk == &diskinfo[i])
	    {
  	        int64_t span;
		if ((PedSector)-1 == reqs[j].max_blk)
		    reqs[j].max_blk = maxmax_blk;
		span = reqs[j].max_blk - reqs[j].min_blk;
		total_wanted += span;
		autopartkit_log(0, "Adding %lld to total_wanted, now %lld\n",
				(long long)span, (long long)total_wanted);
	    }
	}
        if (total_wanted == 0)
            continue;
	/* This is where we decide the new size of the partitions */
	for (j = 0; reqs[j].mountpoint; ++j)
        {
	    if (0 == reqs[j].max_blk)
	        continue; /* Ignore zero-size partitions */
	    if (reqs[j].curdisk == &diskinfo[i])
	    {
		PedSector newsize;
		/* These calculations can overflow if the numbers are
		   too big. */
		newsize = reqs[j].max_blk - reqs[j].min_blk;
		if (newsize)
		{ /* No need to resize if min == max */
		    newsize *= diskinfo[i].freespace;
		    assert(total_wanted);
		    newsize /= total_wanted;
		    newsize += reqs[j].blocks;
		    if (newsize > reqs[j].max_blk)
		        newsize = reqs[j].max_blk;

		    /* We know the new size.  Activate it */
		    total_wanted -= reqs[j].max_blk - reqs[j].min_blk;
		    diskinfo[i].freespace -= newsize - reqs[j].blocks;
		    reqs[j].blocks = newsize;
		}
	    }
	}
    }
    return 0;
}

void
print_list(struct disk_info_t diskinfo[], struct diskspace_req_s reqs[])
{
    int i;
    autopartkit_log(0, "Suggested partition distribution:\n");
    for (i = 0; diskinfo[i].capacity; i++)
    {
        int j;
	autopartkit_log(0, "  free block '%s' [%llu,%llu]:\n",
	       diskinfo[i].path ? diskinfo[i].path : "[null]",
	       BLOCKS_TO_MiB(diskinfo[i].capacity),
	       BLOCKS_TO_MiB(diskinfo[i].freespace));
	for (j = 0; reqs[j].mountpoint; ++j)
	{
	    if (reqs[j].curdisk == &diskinfo[i])
	    {
		autopartkit_log(0,
				"    %s [%llu <= %llu <= %llu] [%lld <= %lld]"
				" %s %lld-%lld %s\n",
				( reqs[j].mountpoint ?
				  reqs[j].mountpoint : "[null]" ),
				BLOCKS_TO_MiB(reqs[j].min_blk),
				BLOCKS_TO_MiB(reqs[j].blocks),
				BLOCKS_TO_MiB(reqs[j].max_blk),
				(uint64_t)reqs[j].minsize,
				(uint64_t)reqs[j].maxsize,
				reqs[j].curdisk->path,
				reqs[j].curdisk->geom.start,
				reqs[j].curdisk->geom.end,
				reqs[j].ondisk ? "on disk" : "");
	    }
	}
    }
    autopartkit_log(1, "Done listing suggested partition distribution.\n");
}

static int
cmp_spaceinfo_path(const void *ap, const void *bp)
{
    const struct disk_info_t *a = (const struct disk_info_t *)ap;
    const struct disk_info_t *b = (const struct disk_info_t *)bp;
    return strcmp(a->path, b->path);
}

struct disk_info_t *
get_free_space_list(void)
{
    PedDevice *dev = NULL;
    PedDisk *disk = NULL;
    PedPartition *part = NULL;

    int spacenum = 0;
    const int spacecapacity = 10;

    struct disk_info_t *spaceinfo = calloc(sizeof(*spaceinfo), spacecapacity);

    autopartkit_log(2, "Locating free space on all disks\n");

    /* Detect all devices */
    ped_device_probe_all();

    /* Loop over the detected devices */
    for (dev = my_ped_device_get_next_rw(NULL); dev; dev = my_ped_device_get_next_rw(dev))
    {
        assert(dev);
	autopartkit_log(2, "  checking dev: %s, sector_size=%d\n",
			dev->path,
			dev->sector_size /* in bytes */);

        disk = ped_disk_new(dev);

	for(part = ped_disk_next_partition(disk, NULL); part;
	    part = ped_disk_next_partition(disk, part))
	{
	    assert(part);

	    autopartkit_log( 2,
			     "    part: %d, type: %d size: (%lld-%lld) %lld\n",
			     part->num, part->type,
			     part->geom.start,part->geom.end,
			     part->geom.length);

	    if ((part->type & PED_PARTITION_FREESPACE) ==
		PED_PARTITION_FREESPACE)
	    {
	        autopartkit_log(2, "    free space %lld\n", part->geom.length);
		/* Store the info we need to find this block again */
		spaceinfo[spacenum].path = disk->dev->path;
		memcpy(&spaceinfo[spacenum].geom, &part->geom,
		       sizeof(part->geom));
		spaceinfo[spacenum].capacity = part->geom.length;
		spaceinfo[spacenum].freespace = part->geom.length;
		++spacenum;
		assert(spacenum < spacecapacity);
	    }
	}
    }
    autopartkit_log(2, "Done locating free space, found %d free areas\n",
		    spacenum);
    if (0 < spacenum)
    {
        /* sort list on device path, order bus0 before bus1 */
        qsort(spaceinfo, spacenum, sizeof(spaceinfo[0]), cmp_spaceinfo_path);
        return spaceinfo;
    }
    else
    {
        free(spaceinfo);
        return NULL;
    }
}

/* andread@linpro.no */
void
reduce_disk_usage_size(struct disk_info_t *vg,
                     struct diskspace_req_s reqs[],
                     double percent){
    /* Reduce free vg space to sum(min values) + percent of free space */
    int i;
    int minimum = 0;
    int newsize = 0;
    for(i=0; i<10 && reqs[i].mountpoint; i++){
      minimum = minimum + MiB_TO_BLOCKS(reqs[i].minsize);
    }
    newsize = ((vg->capacity) - minimum) * percent;
    vg->freespace = newsize + minimum;
}

#ifdef __linux__
static int is_cdrom(const char *path)
{
    int fd;
    int ret;

    fd = open(path, O_RDONLY | O_NONBLOCK);
    ret = ioctl(fd, CDROM_GET_CAPABILITY, NULL);
    close(fd);

    if (ret >= 0) {
        autopartkit_log( 1, "device %s is cdrom\n", path);
	return 1;
    }
    else
	return 0;
}
#else /* !__linux__ */
#define is_cdrom(path) 0
#endif /* __linux__ */

PedDevice *my_ped_device_get_next_rw(PedDevice *dev)
{
    dev = ped_device_get_next(dev);
    while (dev && (dev->read_only || is_cdrom(dev->path))) {
        autopartkit_log( 1, "skipping device %s\n", dev->path);
        dev = ped_device_get_next(dev);
    }
    return dev;
}
