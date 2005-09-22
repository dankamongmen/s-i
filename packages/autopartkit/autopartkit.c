/* Autopartkit -- debian-edu -- customized version of autopartkit
   from debian-installer.

   Author: Tollef Fog Heen <tollef@add.no>
   Copyright (C) 2002 Tollef Fog Heen <tollef@add.no>

   Like autopartkit, the modifications are under the GPL (see under).

*/
/*
   autopartkit.c - Module to partition devices for debian-installer.
   Author - Raphaël Hertzog

   Copyright (C) 2001  Logidée (http://www.logidee.com)
   Copyright (C) 2001  Raphaël Hertzog <hertzog@debian.org>

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

*/

/*
 * Here's the global algorithm used by autopartkit :
 *
 * 1. Detect available disks.
 *
 * 2. Choose all disks:
 *    - automatic if there's only a single disk
 *    - from the list of available disk if more than one disk is available
 *      Provide enough info on the disks to do an informed choice :
 *       Device   | Model         | Size | NbPart | FreeSpace | Space in FAT
 *       -------------------------------------------------------------------
 *       /dev/hda | IBM...        | 30G  | 1      | 0M        | 25G
 *       /dev/hdb | Maxtor...     | 12G  | 6      | 5G        | 3G
 *    - by asking the device name to the user if no disk was found
 *
 *    We clear the disks, so we don't need to worry about free space.
 *
 * 3. Build a list of all the disks we have and allocate optimal
 * amounts to the different partitions.  Also, check whether we are
 * doing a server or workstation installation.
 *
 * 4. load the requested partition sizes from /etc/autopartkit/
 *
 * Look at how much space we've got and try to maximize all the partitions.
 *
 * 6. Create the required partitions in the free space, mount them
 *    on /target and write the entries in /target/etc/fstab.
 *
 */

#include "config.h"

#define _GNU_SOURCE /* To enable vasprintf() */
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <parted/parted.h>
#if defined(TEST)
#include "dummydebconfclient.h"
#else /* not TEST */
#include <cdebconf/debconfclient.h>
#endif /* not TEST */

#include <sys/mount.h>
#include <sys/swap.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <syslog.h>
#include <assert.h>

#include "autopartkit.h"

/*
 * How much of the "excess" LVM space should be spent on the LVM
 * logical volumes.  This accounts for LVM overhead, and make sure
 * some VG space is available for later use.
 */
#define LVM_SPEND_FACTOR (0.5)

#define FAT_MINSIZE_FACTOR 1.33

#define MAX_PARTITIONS 10

#define PART_SIZE_BYTE(device, part) ((part)->geom.length * (device)->sector_size)

/* Write /etc/windows_part?
#define WRITE_WINDOWS_PART
*/


/* Ignore devfs devices, used in choose_dev */
#define IGNORE_DEVFS_DEVICES 1

#if 0
#define log_line() \
  autopartkit_log("  Error bounding: %s %d\n",__FILE__,__LINE__)
#else
#define log_line()
#endif

#include "parted-compat.h"

static struct debconfclient *client;

typedef struct _DeviceStats DeviceStats;
struct _DeviceStats {
    int	    nb_part;
    int	    nb_fat_part;
    int	    size;
    int	    free_space;
    int	    free_space_in_fat;
    int	    max_contiguous_free_space;
    int	    has_extended;
};

/* Pre-declarations */
static const char* mydebconf_input(char*, char*);
static PedDevice* choose_device(void);
static void fix_mounting(device_mntpoint_map_t mountmap[], int partcount);
#if defined(fordebian)
static DeviceStats* get_device_stats(PedDevice*);
#endif /* fordebian */
#if defined(HAVE_PED_DISK_COMMIT)
static void autopartkit_handle_timer(PedTimer* timer, void* context);
#endif

#if defined(HAVE_PED_DISK_COMMIT)
/* update the debconf progress bar when given notice by autopartkit */
/* No longer used after rewrite to use /sbin/mkfs.*. [pere 2005-03-19] */
static void
autopartkit_handle_timer(PedTimer* timer, void* context)
{
    int minutes_left = (timer->predicted_end - timer->now) / 60;
    char hours[16], minutes[16];
    char *template;

    if (minutes_left < 1) {
        template = "autopartkit/createfs-estimate-lessthanaminute";
    } else if (minutes_left < 60) {
        template = "autopartkit/createfs-estimate-nohour";
    } else {
        template = "autopartkit/createfs-estimate";
    }

    sprintf(hours, "%u", minutes_left / 60);
    sprintf(minutes, "%u", minutes_left % 60);

    debconf_subst(client, template, "HOURS", hours);
    debconf_subst(client, template, "MINUTES", minutes);

    debconf_progress_info(client, template);
    debconf_progress_set(client, (int)(timer->frac * 1000.0f));
}
#endif

void
autopartkit_error (int isfatal, const char * format, ...)
{
    const char *template = "autopartkit/error";
    char *msg = NULL;
    va_list ap;

    va_start(ap, format);
    if (-1 == vasprintf(&msg, format, ap))
    {
        /* fatal error */
        exit(EXIT_FAILURE);
    }
    va_end(ap);

    autopartkit_log(1, "error: %d, %s\n", isfatal, msg);
    debconf_fset(client, template, "seen", "false");
    debconf_subst(client, template, "error", msg);
    debconf_input(client, "high", template);
    debconf_go(client);

    if (msg)
        free(msg);

    if (isfatal)
        exit (isfatal);
}

static const char * mydebconf_input (char *priority, char *template)
{
    debconf_fset (client, template, "seen", "false");
    debconf_input (client, priority, template);
    debconf_go (client);
    debconf_get (client, template);
    return client->value;
}

void autopartkit_log(const int level, const char * format, ...)
{
    int LOGLIMIT = 1;
    va_list ap;
    if (level > LOGLIMIT)
        return;

    openlog("autopartkit", LOG_PID, LOG_USER);
    va_start(ap, format);
    vsyslog(LOG_DEBUG, format, ap);
    va_end(ap);
}

/* note: only fsets seen=true if the user answered yes */
static void autopartkit_confirm(void)
{
    const char *template = "autopartkit/confirm";
    debconf_input (client, "critical", template);
    debconf_go (client);
    debconf_get (client, template);
    if (strstr(client->value, "true"))
        return;
    else
    {
	/* Make sure the question is displayed the next time too */
	debconf_fset (client, template, "seen", "false");
        exit(EXIT_FAILURE);
    }
}

static void disable_kmsg(int disable)
{
    static char level[64];
    FILE* printk;
    if (disable)
    {
	int read;
	printk = fopen("/proc/sys/kernel/printk", "r");
	if (! printk)
	{
	    autopartkit_error(0, "Could not read from /proc/sys/kernel/printk");
	    return;
	}
	read = fread(level, 1, 63, printk);
	level[read] = '\0';
	fclose(printk);
	printk = fopen("/proc/sys/kernel/printk", "w");
	if (! printk)
	{
	    autopartkit_error(0, "Could not write to /proc/sys/kernel/printk");
	    return;
	}
	fprintf(printk, "0\n");
	fclose(printk);
    } else {
	printk = fopen("/proc/sys/kernel/printk", "w");
	if (! printk)
	{
	    autopartkit_error(0, "Could not write to /proc/sys/kernel/printk");
	    return;
	}
	fprintf(printk, "%s", level);
	fclose(printk);
    }
}

/**
 * Return a string representing the defalt partition type for the
 * current hardware
 */
static const char *
default_disk_label(void)
{
    /* Need to define on a per arch basis */
#if defined(__i386__)
    return  "msdos";
#elif defined(ia64)
    return "GPT";
#elif defined(hppa)
    return"msdos";
#elif defined(__sparc__)
    return "sun";
#elif defined(__mips__) && defined(__MIPSEL__)
    return"msdos";
#elif defined(__mips__) && defined(__MIPSEB__)
/* Only supported in libparted 1.6.3? */
    return "mips"; /* SGI disklabel */
#else /* not __i386__ */
#  warning "Default DISK_LABEL is not known or not supported on this platform"
    return "msdos";
#endif /* not __i386__ */
}

/**
 * Report true if the mount point is /, and false if it isn't.
 */
static int is_root(const char *mountpoint)
{
    return (0 == strcmp(mountpoint, "/") );
}

/*
 * Step 1 & 2: Discover and select the device
 *
 * Probe the available device. Automatically select it, if one is
 * available. Ask user input if none is detected.
 *
 */

#if defined(fordebian)
#define TABLE_SIZE 2048
#define LIST_SIZE 512
#endif /* fordebian */

static PedDevice* choose_device(void)
{
    PedDevice *dev = NULL;
    const char *dev_name = NULL;
    int	       nb_try = 0;
#if defined(fordebian)
    char       device_list[LIST_SIZE], default_device[128], table[TABLE_SIZE];
    char      *ptr_list, *ptr_table;
    DeviceStats *stats;
#endif /* fordebian */

    disable_kmsg(1);
    ped_device_probe_all();
    disable_kmsg(0);

    dev = ped_device_get_next(NULL);

    if (dev == NULL)
    {
	/* No devices detected */
	while (dev == NULL)
	{
	    dev_name = mydebconf_input("critical", "autopartkit/device_name");
	    disable_kmsg(1);
	    dev = ped_device_get(dev_name);
	    disable_kmsg(0);
	    if (++nb_try > 2) {
		/* We didn't manage to choose a device */
		break;
	    }
	}
	return dev;
    }
#if defined(fordebian)
    if (dev->next == NULL)
    {
	/* There's only a single device detected */
	return dev;
    }


    default_device[0] = '\0';
    device_list[0] = '\0';
    ptr_list = device_list;
    ptr_table = table;
    for(; dev; dev = ped_device_get_next(dev))
    {
#ifdef IGNORE_DEVFS_DEVICES
	if (strstr(dev->path, "dev/ide/") || strstr(dev->path, "dev/scsi/"))
	    continue;
#endif
	stats = get_device_stats(dev);
	/*
	 * This is the right way to do unfortunately I want insecable
	 * space to override autowrap of debconf ...
	ptr_table += snprintf(ptr_table, TABLE_SIZE + table - ptr_table,
		"%-10s%-20s%05dM %05dM %05dM %-d\n .\n ", dev->path,
		dev->model, stats->size, stats->free_space,
		stats->free_space_in_fat, stats->nb_part);
	*/
	ptr_table += snprintf(ptr_table, TABLE_SIZE + table - ptr_table,
		"%s", dev->path);
	if (10 > strlen(dev->path))
	{
	    memset(ptr_table, ' ', 10 - strlen(dev->path));
	    ptr_table += 10 - strlen(dev->path);
	}
	ptr_table += snprintf(ptr_table, TABLE_SIZE + table - ptr_table,
		"%s", dev->model);
	if (20 > strlen(dev->model))
	{
	    memset(ptr_table, ' ', 20 - strlen(dev->model));
	    ptr_table += 20 - strlen(dev->model);
	}

	ptr_table += snprintf(ptr_table, TABLE_SIZE + table - ptr_table,
		"%05dM %05dM %05dM  %-d\n",
		stats->size, stats->free_space,
		stats->free_space_in_fat, stats->nb_part);

	if (strlen(device_list))
	{
	    ptr_list += snprintf(ptr_list, LIST_SIZE + device_list - ptr_list,
		    ", %s", dev->path);
	} else {
	    ptr_list += snprintf(ptr_list, LIST_SIZE + device_list - ptr_list,
		    "%s", dev->path);
	}
	if (! strlen(default_device))
	{
	    strncpy(default_device, dev->path, 128);
	}
	free(stats);
    }

    dev = NULL;
    nb_try = 0;
    while (dev == NULL)
    {
        const char *template = "autopartkit/choose_device";
        const char *value;
        debconf_fget(client, template, "seen");
        if (strcmp(client->value, "false") == 0)
	    debconf_set(client, template, default_device);
        debconf_subst(client, template, "CHOICES", device_list);
        debconf_subst(client, template, "TABLE", table);

        value = mydebconf_input("critical", template);
        disable_kmsg(1);
        dev = ped_device_get(value);
        disable_kmsg(0);
        if (++nb_try > 3)
            break;
    }
#endif /* fordebian */

    /* Return the first device */
    return dev;
}

static const char *
linux_fstype_to_parted(const char *linux_fstype)
{
    if (strcmp(linux_fstype,"swap") == 0)
        return "linux-swap";
#if defined(LVM_HACK)
    if (strcmp(linux_fstype,"lvm") == 0)
        /* Creating with any FS/partition type and converting to LVM below */
        return "linux-swap"; /* Use any format that is fast to create. */
#endif /* LVM_HACK */
    /* These file systems are not supported by libparted.  create ext2
       and recreate with the correct file system later.  Can't create
       swap here, as the partition table code would be wrong. */
    if (strcmp(linux_fstype,"ext3") == 0)
        return "ext2";
    if (strcmp(linux_fstype,"xfs") == 0)
        return "ext2";
    if (strcmp(linux_fstype,"jfs") == 0)
        return "ext2";
    if (strcmp(linux_fstype,"reiserfs") == 0)
        return "ext2";

    return linux_fstype; /* Pass through everything we don't understand */
}

#if defined(fordebian)
/*
 * Returns a malloced block of stats concerning the indicated device.
 *
 */
static DeviceStats* get_device_stats(PedDevice* dev)
{
    DeviceStats       *stats;
    PedDisk	      *disk;
    PedPartition      *part;
    const PedFileSystemType *fs_type;
    PedFileSystem     *fs;
    PedConstraint     *constraint;

    if (! dev)
	return NULL;

    stats = malloc(sizeof(DeviceStats));
    if (stats == NULL)
	return NULL;

    stats->nb_part = 0;
    stats->nb_fat_part = 0;
    stats->size = 0;
    stats->free_space = 0;
    stats->free_space_in_fat = 0;
    stats->max_contiguous_free_space = 0;
    stats->has_extended = 0;

    stats->size = (PedSector) ((dev->length * dev->sector_size) / MEGABYTE);
    disk = ped_disk_new(dev);
    if (! disk)
    {
	/* There's no partition table */
	stats->free_space = stats->size;
	stats->max_contiguous_free_space = stats->size;
	return stats;
    }

    for(part = ped_disk_next_partition(disk, NULL); part;
	part = ped_disk_next_partition(disk, part))
    {
	if (part->type == PED_PARTITION_EXTENDED)
	{
	    stats->has_extended = 1;
	    continue;
	}
	if (part->type == PED_PARTITION_FREESPACE)
	{
	    PedSector space = (PedSector)(PART_SIZE_BYTE(dev, part) / MEGABYTE);
	    if (space > stats->max_contiguous_free_space)
		stats->max_contiguous_free_space = space;
	    stats->free_space += space;
	    continue;
	}
	if (part->type == PED_PARTITION_PRIMARY ||
	    part->type == PED_PARTITION_LOGICAL)
	{
	    stats->nb_part++;
	    fs_type = part->fs_type;
	    if (! fs_type)
		fs_type = ped_file_system_probe(&(part->geom));
	    if (! fs_type)
	    {
		/* Unknown fs type on partition, ignore partition */
		continue;
	    }
	    /* I hope to catch all FAT* partitions with that */
	    if (strstr(fs_type->name, "fat") || strstr(fs_type->name, "FAT"))
	    {
		stats->nb_fat_part++;
		fs = ped_file_system_open(&(part->geom));
		if (! fs)
		    continue;
		constraint = ped_file_system_get_resize_constraint(
						(const PedFileSystem*) fs);
		if (! constraint)
		    continue;
		/* Only consider as free_space the space that is left when
		 * we take min_size + 33% since we want 25% free on the
		 * resized fat partitition.
		 * FIXME: empty fat partition will be resized to zero-sized
		 * partition, detect those and resize them to a half or a
		 * third instead */
		if (part->geom.length > (PedSector) (constraint->min_size *
			    FAT_MINSIZE_FACTOR))
		{
		    stats->free_space_in_fat += ((part->geom.length -
			(PedSector) (constraint->min_size * FAT_MINSIZE_FACTOR))
			* dev->sector_size) / MEGABYTE;
		}
		ped_file_system_close(fs);

#ifdef WRITE_WINDOWS_PART
		/* Check if it's a windows bootable partition */
		if (ped_partition_is_flag_available(part, boot_flag) &&
		    ped_partition_get_flag(part, boot_flag))
		{
		    winpart = fopen("/etc/windows_part", "w");
		    fprintf(winpart, "%s%d", dev->path, part->num);
		    fclose(winpart);
		}
#endif /* WRITE_WINDOWS_PART */
	    }
	}
    }
    ped_disk_destroy(disk);
    return stats;
}

/*
 * Shrink all FAT partitions in order to free space.
 *
 */
static int shrink_fat_partitions(PedDisk *disk)
{
    PedPartition *part;
    PedFileSystem *fs;
    PedFileSystemType *fs_type = NULL;
    PedConstraint *constraint;

    /* Iterate over all FAT partitions and resize them */
    for(part = ped_disk_next_partition(disk, NULL); part;
	part = ped_disk_next_partition(disk, part))
    {
	if (part->type != PED_PARTITION_LOGICAL)
	    continue;

	fs_type = (PedFileSystemType*) part->fs_type;
	if (! fs_type)
	    fs_type = ped_file_system_probe(&part->geom);
	if (! fs_type)
	    continue;
		
	if (strstr(fs_type->name, "fat") || strstr(fs_type->name, "FAT"))
	{
	    fs = ped_file_system_open(&part->geom);
	    if (! fs)
		continue;
	    constraint = ped_file_system_get_resize_constraint(
						(const PedFileSystem*) fs);
		
	    /* Well, we can use a timer feature of file_system_resize, but now
	     * it's NULL
	     */
	    if ((! ped_disk_set_partition_geom(disk, part, constraint,
		     part->geom.start, part->geom.start +
		     (PedSector)(constraint->min_size * FAT_MINSIZE_FACTOR)))
	        ||
		(! ped_file_system_resize(fs, &part->geom, NULL))
	       )
	    {
		autopartkit_error(0, "Error while resizing a FAT partition.");
		ped_file_system_close(fs);
		ped_constraint_destroy(constraint);
		return 0;
	    }
	    ped_file_system_close (fs);
	    ped_partition_set_system (part, fs_type);	
	}
    }
    ped_disk_commit(disk);
    return 1;
}
#endif /* fordebian */

static char*
find_partition_by_mountpoint(device_mntpoint_map_t *mountmap,
			     char *mountpoint)
{
    int i;
    for (i = 0; i < MAX_PARTITIONS; i++)
    {
        if (mountmap[i].mountpoint == NULL)
	    return NULL;
	if (strcmp(mountmap[i].mountpoint->mountpoint,mountpoint) == 0)
	{
	    return mountmap[i].devpath;
	}
    }
    return NULL;
}

/* Sort diskspace_req_t array by minsize */
static int
dr_minsize_compare(const void *ap, const void *bp)
{
    const diskspace_req_t *a = (const diskspace_req_t *)ap;
    const diskspace_req_t *b = (const diskspace_req_t *)bp;
    return b->minsize - a->minsize;
}

/*
 * Copies requirements while making sure the root (/) is the first
 * entry in the table, and that minsize is smaller or equal to
 * maxsize.  It will also translate fstype "default" to the current
 * default file system type.
 */
static void
normalize_requirements(diskspace_req_t *dest, const diskspace_req_t *source,
		       const size_t dest_capacity)
{
    int i;
    int count;
    memset(dest,0,sizeof(diskspace_req_t)*dest_capacity);

    for (count = 0; source[count].mountpoint != NULL; count++)
    {
        memcpy(&dest[count], &source[count], sizeof(dest[0]));
        dest[count].mountpoint = strdup(source[count].mountpoint);

	if (strcmp(source[count].fstype,"default") == 0)
	    dest[count].fstype = strdup(DEFAULT_FS);
	else
	    dest[count].fstype = strdup(source[count].fstype);

        /* Make sure minsize <= maxsize, unless maxsize == -1 (unlimited) */
	if (-1 != dest[count].maxsize)
	{
	    if (dest[count].minsize > dest[count].maxsize)
	        dest[count].maxsize = dest[count].minsize;
	    if (dest[count].maxsize < dest[count].minsize)
	        dest[count].minsize = dest[count].maxsize;
	}
    }

    if ( ! is_root(dest[0].mountpoint) )
    { /* Move root to the first position (dest[0]) */
        for (i = 0; dest[i].mountpoint != NULL; i++)
	    if ( is_root(dest[i].mountpoint) )
	    {
	        /* swap the entries */
	        diskspace_req_t tmp;
		memcpy(&tmp,     &dest[0], sizeof(tmp));
		memcpy(&dest[0], &dest[i], sizeof(tmp));
		memcpy(&dest[i], &tmp,     sizeof(tmp));
	    }
    }

    /* Sort the rest by minsize, the largest first */
    qsort(&dest[1], count - 1, sizeof(dest[0]),  dr_minsize_compare);
}


/*
 * This hurts my heart, but libparted seems to have some design
 * issues, and it is _impossible_ to get the path of a partition
 * without guessing like this.  That sucks. :(
 *
 * Example paths:
 *  /dev/rd/disc0/disc
 *  /dev/ide/host0/bus0/target0/lun0/disc
 *
 * In parted v1.6.1 we can use ped_partition_get_path(freepart);
 */
static char *get_device_path(PedDevice *dev, PedPartition *freepart)
{
#if defined(HAVE_PED_PARTITION_GET_PATH)
    return ped_partition_get_path(freepart);
#else /* not HAVE_PED_PARTITION_GET_PATH */
    char *retval;
    char *tmp;
    size_t slen;

    asprintf(&retval, "%s%d", dev->path, freepart->num);

    slen = strlen(dev->path);
    tmp = retval + slen - 5; /* 5 is the length of "/disc" */

    printf("tmp=%s\n", tmp);

    /* Replace '/disc' at the end with 'part'.  It sucks having to do
       it this way. */
    if (0 == strncmp("/disc", tmp, 5))
    {
        tmp[1] = 'p';
        tmp[2] = 'a';
        tmp[3] = 'r';
        tmp[4] = 't';
    }
    return retval;
#endif /* not HAVE_PED_PARTITION_GET_PATH */
}

/*
 * Make mounting points, and mount the newly created filesystems.
 * Create fstab in /target/fstab.
 */
static void
fix_mounting(device_mntpoint_map_t mountmap[], int partcount)
{
    int i;

    /* Mount partitions and write fstab */
    if (0 != mkdir("/target", 0755))
        autopartkit_error(1, "Unable to mkdir /target: %s", strerror(errno));

    /* Find and mount the root fs */

    autopartkit_log( 1, "device for /: %s\n",
		     normalize_devfs(find_partition_by_mountpoint(mountmap,"/")));

    /* FIXME Should use fstype for /, not DEFAULT_FS */
    if (mount(find_partition_by_mountpoint(mountmap,"/"),
	      "/target", DEFAULT_FS, MS_MGC_VAL, NULL) == -1)
        autopartkit_error(1,
			  "Unable to mount '%s' on '/target' as fstype '%s': %s",
			  find_partition_by_mountpoint(mountmap,"/"),
			  DEFAULT_FS, strerror(errno));
    log_line();

    /* Find and turn on swap */
    {
        char *partition = find_partition_by_mountpoint(mountmap,"swap");
	if (partition)
	{
	    disable_kmsg(1);
	    swapon(partition, 0);
	    disable_kmsg(0);
	}
	else
	    autopartkit_log(0, "No swap partition available!\n");
    }
    log_line();

    /* Are these really needed?  Who will create them if they are missing? */
    if (0 != mkdir("/target/floppy", 0755))
        autopartkit_error(1, "Unable to mkdir /target/floppy: %s",
			  strerror(errno));
    if (0 != mkdir("/target/cdrom", 0755))
        autopartkit_error(1, "Unable to mkdir /target/cdrom: %s",
			  strerror(errno));
    for (i = 0; i < partcount; i++)
    {
        char *tmpmnt;
	char *devpath;
	if ( is_root(mountmap[i].mountpoint->mountpoint)
#if defined(LVM_HACK)
	     || (strcmp(mountmap[i].mountpoint->fstype,"lvm") == 0)
#endif /* LVM_HACK */
	     || (strcmp(mountmap[i].mountpoint->fstype,"swap") == 0))
	{
	    continue;
	}
	autopartkit_log( 1, "Mounting %s on %s as fs %s.\n",
			 mountmap[i].devpath ? mountmap[i].devpath : "[null]",
			 ( mountmap[i].mountpoint->mountpoint ?
			   mountmap[i].mountpoint->mountpoint : "[null]" ),
			 mountmap[i].mountpoint->fstype);

	devpath = normalize_devfs(mountmap[i].devpath);

	asprintf(&tmpmnt, "/target%s", mountmap[i].mountpoint->mountpoint);
	make_path(tmpmnt, 0755);
	if (mount(mountmap[i].devpath, tmpmnt, mountmap[i].mountpoint->fstype,
		  MS_MGC_VAL, NULL) == -1)
	    autopartkit_error(1,
			      "Unable to mount '%s' on '%s' as fstype '%s': %s",
			      mountmap[i].devpath, tmpmnt,
			      mountmap[i].mountpoint->fstype,
			      strerror(errno));
	free(tmpmnt);
    }

    /* Make sure /tmp got sticky bit.  It must be changed on the
       mounted device after it is mounted. */
    chmod("/target/tmp", 01777);

}

/*
 * Remove all partitions on all detected disks.
 * FIXME: Do not work with libparted 1.6
 */
static void
nuke_all_partitions(void)
{
    PedDevice *dev;

    /* Loop over all devices, and nuke the partition table on each one. */
    dev = ped_device_get_next(NULL);
    do {
        PedDisk *p;
	p = ped_disk_new_fresh(dev, ped_disk_type_get(default_disk_label()));
#if defined(HAVE_PED_DISK_COMMIT) /* libparted 1.6 */
	ped_disk_commit(p);
#endif
	ped_disk_destroy(p);
	dev = ped_device_get_next(dev);
    } while (dev != NULL);
}

/*
 * Zero out the start of a device, filling 'length' bytes with
 * 0-bytes.  This is used to remove old headers from LVM partitions.
 */
static int
zero_dev(const char *devpath, unsigned int length)
{
    FILE *fp = fopen(devpath, "w");
    char buf[10240]; /* 10 KiB blocks */
    unsigned int i;

    if (!fp)
        return -1;

    memset(buf, 0, sizeof(buf));
    for (i = 0; i < length; )
    {
        unsigned int size = length - i;
	if (size > sizeof(buf))
	    size = sizeof(buf);

        fwrite(buf, size, 1, fp);

	i += size;
    }
    fclose(fp);
    return 0;
}

/*
 * Create a filesystem on the device pointed to by devpath.
 */
static int
makefs(const char *devpath, const char *fstype)
{
    char *mkfs;
    int retval;
    char *cmd = NULL;

    assert(devpath);
    assert(fstype);
    /* Create filesystem */
    /* Do it like this for now */
    if (0 == strcmp("swap", fstype))
        mkfs = "/sbin/mkswap";
    /* XXX This should accept any fstype string, and just use
       /sbin/mkfs.<fstype> if it exist. */
    else if (0 == strcmp("ext2", fstype))
        mkfs = "/sbin/mkfs.ext2";
    else if (0 == strcmp("ext3", fstype))
        mkfs = "/sbin/mkfs.ext3";
    else if (0 == strcmp("jfs", fstype))
        mkfs = "/sbin/mkfs.jfs";
    else if (0 == strcmp("xfs", fstype))
        mkfs = "/sbin/mkfs.xfs";
    else if (0 == strcmp("reiserfs", fstype))
        mkfs = "/sbin/mkfs.reiserfs";
    else /* Any default fs will have to do for now */
      {
	autopartkit_log(1, "  Unknown fs '%s' for dev '%s'\n", fstype,devpath);
	return -1;
      }
    retval = asprintf(&cmd, "log-output -t autopartkit %s %s", mkfs, devpath);
    if (-1 != retval)
      {
	autopartkit_log(1, "Running command: %s\n", cmd);
	retval = system(cmd);
	free(cmd);
	if (0 != retval)
	    autopartkit_error(1, "  Failed to create '%s' fs on '%s'",
			      fstype, devpath);
	else
	  return 0;
      }
    else
        autopartkit_error(1, "Unable to allocate memory for mkfs call");
    return -1;
}

/*
 * Create all the partitions on the disk
 *
 */
static void
make_partitions(const diskspace_req_t *space_reqs, PedDevice *devlist)
{
    int ret;
    int partnum;
    PedPartition *newpart = NULL;
    PedConstraint *any;
    PedFileSystemType *fs_type = NULL;
    PedDevice *dev_tmp = NULL;
    PedDisk *disk_tmp = NULL, *disk_maybe = NULL;
    PedFileSystem *fs;
    device_mntpoint_map_t mountmap[MAX_PARTITIONS];
    diskspace_req_t requirements[MAX_PARTITIONS], *req_tmp = NULL;
    int partcount = 0;
    struct disk_info_t *spaceinfo = NULL;

#if defined(HAVE_PED_DISK_COMMIT)
    PedTimer *timer = NULL;
#endif /* HAVE_PED_DISK_COMMIT */

#if defined(LVM_HACK)
    void *lvm_pv_stack;
    void *lvm_lv_stack;
    void *lvm_vg_stack;
#endif /* LVM_HACK */

    memset(mountmap,0,sizeof(device_mntpoint_map_t)*MAX_PARTITIONS);

    normalize_requirements(requirements, space_reqs, MAX_PARTITIONS);

#if defined(LVM_HACK)
    lvm_pv_stack = lvm_pv_stack_new();
    lvm_lv_stack = lvm_lv_stack_new();

    autopartkit_log(1, "Created LVM stacks\n");

    /* Do not make LVM logical volumes on the disk */
    for (partnum = 0; partnum < MAX_PARTITIONS && requirements[partnum].fstype;
	 ++partnum)
        if ( 0 == strncmp("lvm:", requirements[partnum].fstype, 4) )
	    requirements[partnum].ondisk = 0;
#endif /* LVM_HACK */

    spaceinfo = get_free_space_list();
    if ( ! spaceinfo )
        /* fatal error */
        autopartkit_error (1,"  Unable to find any free space\n");

    autopartkit_log(1, "Found free space, distributing partitions.\n");
    ret = distribute_partitions(spaceinfo, requirements);

    if (ret)
        autopartkit_error(1, "partitioning failed, retval=%d\n", ret);
    else
    {
	autopartkit_log(1, "distribute_partition ok.\n");
	print_list(spaceinfo, requirements);
    }

    /*
     * The root ('/') will be the first partition in this list after the
     * call to normalize_requirements().
     */
    for (partnum = 0; partnum < MAX_PARTITIONS && requirements[partnum].fstype;
	 ++partnum)
    {
        req_tmp = &requirements[partnum];

	autopartkit_log(1, "Making '%s', using %d MiB, minsize=%d\n",
			req_tmp->mountpoint,
			(int)BLOCKS_TO_MiB(req_tmp->blocks),
			(int)req_tmp->minsize);
	if (req_tmp->minsize == -1)
	    break;

	assert(req_tmp->fstype);
	{ /* Look up the file system type with libparted. */
	    const char *parted_fs = linux_fstype_to_parted(req_tmp->fstype);
	    assert(parted_fs);
	    fs_type = ped_file_system_type_get(parted_fs);
	}

	autopartkit_log(1, "  mountpoint: %s fstype %s\n", req_tmp->mountpoint,
			req_tmp->fstype);
	if (NULL == fs_type)
	{
	    char *devpath = NULL;
	    autopartkit_log(1, "  fstype '%s' not handle by libparted.\n",
			    req_tmp->fstype);
#if defined(LVM_HACK)
	    if ( 0 == strncmp("lvm:", req_tmp->fstype, 4) )
                devpath = lvm_lv_add(lvm_lv_stack, req_tmp->fstype,
                                     req_tmp->minsize, req_tmp->maxsize);
	    else
#endif /* LVM_HACK */
	    {
		autopartkit_log(1, "  Passing it directly to fstab.\n");
		devpath = strdup("none");
	    }
	    /* Add to fstab */
	    mountmap[partcount].devpath = devpath;
	}
	else
	{
	    int isroot = 0;
	    PedSector endsector;

	    assert(req_tmp->curdisk); /* XXX fails for shmfs, ie ->nodisk */

	    autopartkit_log(1, "  device path '%s' [%lld-%lld]\n",
			    req_tmp->curdisk->path,
			    req_tmp->curdisk->geom.start,
			    req_tmp->curdisk->geom.end);

	    dev_tmp = ped_device_get(req_tmp->curdisk->path);
	    assert(dev_tmp);
	    disk_maybe = ped_disk_new(dev_tmp);

	    autopartkit_log(1, "  creating in free area %lld-%lld\n",
			    req_tmp->curdisk->geom.start,
			    req_tmp->curdisk->geom.end);

	    autopartkit_log(1, "  Creating partition on disk having "
			    "sectors %lld\n", disk_maybe->dev->length);
	    any = PED_CONSTRAINT_ANY(disk_maybe, disk_maybe->dev);
	    autopartkit_log(1, "  Trying to create part on %lld-%lld\n",
			    req_tmp->curdisk->geom.start,
			    req_tmp->curdisk->geom.end);

	    isroot = is_root(req_tmp->mountpoint);

	    if (!isroot && !ped_disk_extended_partition(disk_maybe))
	    {
	        PedPartition *ext_part;
		autopartkit_log(1, "  **Creating extended partition: %lld %lld\n",
				req_tmp->curdisk->geom.start,
				req_tmp->curdisk->geom.end);

		/* Create extended partition filling the complete free
		   space area. */
		ext_part = ped_partition_new(disk_maybe,
					     PED_PARTITION_EXTENDED,
					     fs_type,
					     req_tmp->curdisk->geom.start,
					     req_tmp->curdisk->geom.end);
		ret = ped_disk_add_partition(disk_maybe, ext_part, any);
		if ( 0 == ret)
		    /* fatal error */
		    autopartkit_error (1, "  ped_disk_add_partition failed\n");

		ped_disk_maximize_partition(disk_maybe, ext_part, any);
	    }

	
	    /* blocks -1 because the start and end numbers are inclusive */
	    endsector = req_tmp->curdisk->geom.start + req_tmp->blocks - 1;

	    /* Make sure we stay inside the free space geometry */
	    if (endsector > req_tmp->curdisk->geom.end)
	    {
		autopartkit_log(1, "  end sector value %lld to large, "
				"trunkating to %lld\n",
				endsector, req_tmp->curdisk->geom.end);

		endsector = req_tmp->curdisk->geom.end;
	    }

	    autopartkit_log(1, "  New partition from %lld to %lld\n",
			    req_tmp->curdisk->geom.start, endsector);

	    newpart = ped_partition_new( disk_maybe,
					 ( isroot ? PED_PARTITION_PRIMARY
					   : PED_PARTITION_LOGICAL ),
					 fs_type,
					 req_tmp->curdisk->geom.start,
					 endsector );
	
	    if (isroot && ped_partition_is_flag_available(newpart,
							  PED_PARTITION_BOOT))
	        ped_partition_set_flag(newpart,PED_PARTITION_BOOT,1);

	    ret = ped_disk_add_partition(disk_maybe, newpart, any);

	    mountmap[partcount].devpath =
	      get_device_path(disk_maybe->dev, newpart);

	    autopartkit_log(1, "  ped_disk_add_partition: %d\n", ret);
	    if (0 != ret)
	    {

	        /* Mark the used area as occupied in the common struct. */
	        req_tmp->curdisk->geom.length -=
		  newpart->geom.end - newpart->geom.start; /* is this needed?*/
	        req_tmp->curdisk->geom.start = newpart->geom.end + 1;

                /*
                 * We must commit before creating file system to make
                 * sure the device file is available when we need it.
                */
                ped_disk_commit(disk_maybe);

                makefs(mountmap[partcount].devpath, req_tmp->fstype);
	    }
	    else
	    {
	        /* fatal error */
	        autopartkit_error (1,"  ped_disk_add_partition failed\n");
	    }
		
	    ped_constraint_destroy(any);

#if defined(LVM_HACK)
	    /*
	     * It is currently not possible to create LVM partitions,
	     * but it is possible to create an partition with another
	     * filesystem, and then change the partition type to LVM.
	     */
	    if (strcmp(req_tmp->fstype,"lvm") == 0)
	    {
	        const char *devpath;

	        autopartkit_log(1, "  converting partition type to LVM\n");
		ped_partition_set_flag(newpart,PED_PARTITION_LVM,1);

	        devpath = mountmap[partcount].devpath;

		/* We must commit before calling lvm_init_dev() to
		   make sure the device file is available when we need
		   it. */
		ped_disk_commit(disk_maybe);

		lvm_pv_stack_push(lvm_pv_stack, req_tmp->mountpoint, devpath);
	    }
	    else
#endif /* LVM_HACK */
	    {
	        /* disable_kmsg(1); */
	        log_line();
		/* disable_kmsg(0); */
		ped_disk_commit(disk_maybe);
	    }
	}
	mountmap[partcount].mountpoint = req_tmp;

	autopartkit_log( 1, "  mp: %s dev: %s\n",
			 req_tmp->mountpoint,
			 mountmap[partcount].devpath);

	newpart = NULL;
	/* req_tmp->minsize = -1; */ /* Why? */
	partcount++;
    }

    /* Workaround for stupid bug in libparted.  It closes the device as well
       as the disk when ped_disk_close is called.  This means that the
       device is in an unknown state when we return from this function, or
       rather, it is closed when returning.  This is _bad_. */
    for (dev_tmp = devlist; dev_tmp;
	 dev_tmp = ped_device_get_next(dev_tmp))
    {
        log_line();
	disk_tmp = ped_disk_new(dev_tmp);
	log_line();
	ret = ped_disk_commit(disk_tmp);
	log_line();
	autopartkit_log(1, "ped_disk_commit(disk_tmp) = %d\n",ret);
	ped_disk_destroy(disk_tmp);
	log_line();
    }

    /* Finish the work */
    disable_kmsg(1);
    /* Close libparted, needs to do it here just to let the kernel
     * know of the new partition table so that mounts call actually work
     */
    PED_DONE();
    disable_kmsg(0);

    free(spaceinfo);

#if defined(LVM_HACK)
    /* Initialize LVM partitions and volumes, if the LVM tools are available */
    autopartkit_log(1, "Initializing LVM.\n");

    while ( ! lvm_pv_stack_isempty(lvm_pv_stack) )
    {
        char *vgname;
        char *devpath;
        lvm_pv_stack_pop(lvm_pv_stack, &vgname, &devpath);
	autopartkit_log(1, "  Init LVM pv name %s, devpath=%s\n",
			vgname, devpath);

        /*
         * Zero out old LVM headers if present.  Is 100 KiB a good
         * value?
         */
        zero_dev(devpath, 100 * 1024);

        if ( 0 == lvm_init_dev(devpath) )
        {
            autopartkit_log(1, "  lvm_init_dev(%s) successful.\n", devpath);
            lvm_volumegroup_add_dev(vgname, devpath);
	}
	else
            autopartkit_log(1, "  lvm_init_dev(%s) failed.\n", devpath);
        free(vgname);
        free(devpath);
    }

    /* Distribute logical volumes (andread@linpro.no) */
    lvm_vg_stack = lvm_vg_stack_new();

    /* Store vgnames on stack */
    for (partnum = 0; partnum < MAX_PARTITIONS && requirements[partnum].fstype;
	 ++partnum)
    {
        if ( 0 == strncmp("lvm\0", requirements[partnum].fstype, 4) ){
	    lvm_vg_stack_push(lvm_vg_stack, requirements[partnum].mountpoint);
      }
    }

    /* Loop vg-stack, get disk_info and create request-array */
    while ( ! lvm_vg_stack_isempty(lvm_vg_stack) ){
        struct disk_info_t vg[2];
        struct diskspace_req_s lvm_reqs[MAX_PARTITIONS];
        int lvm_reqs_index = 0;
        char *vgname;

        lvm_vg_stack_pop(lvm_vg_stack, &vgname);
        if ( ! lvm_get_free_space_list(vgname, &vg[0]) ){
            printf("ERROR: No vg %s\n", vgname);
            continue;
        }
        vg[1].capacity = 0;
        /* loop requests... */
        for (partnum = 0; partnum < MAX_PARTITIONS &&
	       requirements[partnum].fstype;
	     ++partnum){
            char *identmatch;
            char *elements[4];

            asprintf(&identmatch, "lvm:%s:", vgname);
            if ( 0 == strncmp(identmatch, requirements[partnum].fstype,
                              strlen(identmatch) ) ) {
                /* Insert into request array */
                lvm_split_fstype(requirements[partnum].fstype, ':',
				 4, elements);
		if (0 == strcmp("default", elements[3])) {
		    char *s = strdup(DEFAULT_FS);
		    assert(s);
		    free(elements[3]);
		    elements[3] = s;
		}

                lvm_reqs[lvm_reqs_index].mountpoint = elements[2];
                lvm_reqs[lvm_reqs_index].ondisk = 1;
                lvm_reqs[lvm_reqs_index].fstype = elements[3];
                lvm_reqs[lvm_reqs_index].minsize =
		    requirements[partnum].minsize;
                lvm_reqs[lvm_reqs_index].maxsize =
		    requirements[partnum].maxsize;
		autopartkit_log(2, "  LVM request array: %s, %d, %d\n",
				lvm_reqs[lvm_reqs_index].mountpoint,
				lvm_reqs[lvm_reqs_index].minsize,
				lvm_reqs[lvm_reqs_index].maxsize);
                lvm_reqs_index++;
            }
            lvm_reqs[lvm_reqs_index].mountpoint = NULL;
            free(identmatch);
        }

        /* Reduce vg size by LVM_SPEND_FACTOR... */
	autopartkit_log(1, "Vg %s - %d to use\n",
			vg[0].path, vg[0].freespace);
	autopartkit_log(1, "Reducing...\n");
        reduce_disk_usage_size(&vg[0], lvm_reqs, LVM_SPEND_FACTOR);
	autopartkit_log(1, "Vg %s - %d to use\n",
			vg[0].path, vg[0].freespace);
        /* Call distribute_partitions */
	autopartkit_log(1, "  LVM distribute_partitions...\n");
        distribute_partitions(vg, lvm_reqs);
	autopartkit_log(1, "Layout lits:\n");
        print_list(vg, lvm_reqs);
        /* Make logical volumes */
        for (partnum = 0; partnum < 10 &&
               lvm_reqs[partnum].mountpoint; ++partnum){
            char *lvname;
            int mbsize;
            char *devpath = NULL;
            char *fstype;

            lvname = lvm_reqs[partnum].mountpoint;
            mbsize = BLOCKS_TO_MiB(lvm_reqs[partnum].blocks);
	    fstype = lvm_reqs[partnum].fstype;
	
	    autopartkit_log(2, "  LVM creating %s, %s, %d\n",
			    vgname, lvname, mbsize);
	    devpath = lvm_create_logicalvolume(vgname, lvname, mbsize);
	    if (NULL == devpath)
	        autopartkit_log(1, "  LVM lv creation failed\n");
	    else
	      {
		autopartkit_log(1, "  LVM lv created ok, devpath=%s\n",
				devpath);
		autopartkit_log(1, "  LVM creating fs: %s\n", fstype);
		if (0 == makefs(devpath, fstype))
		  { /* Replace devpath placeholder with real path */
		    char buf[1024];
		    int i;
		    autopartkit_log(1, "  FS '%s' created ok on '%s'",
				    fstype, devpath);
		    snprintf(buf, sizeof(buf), "%s:%s", vgname, lvname);
		    for (i = 0; i < partcount; i++)
		      if (0 == strcmp(buf, mountmap[i].devpath))
			{
			  free(mountmap[i].devpath);
			  mountmap[i].mountpoint->fstype = strdup(fstype);
			  mountmap[i].devpath = devpath;
			}
		  }
		else
		    autopartkit_log(1, "  FS creation failed");
	      }
	}
    }
    autopartkit_log(1, "LVM done\n");

    lvm_lv_stack_delete(lvm_lv_stack);
    lvm_pv_stack_delete(lvm_pv_stack);
#endif /* LVM_HACK */

    fix_mounting(mountmap, partcount);

    log_line();
}

/*
 * This exception handler is required to not let libparted write warnings
 * on stdout and confuse cdebconf.
 *
 */
static PedExceptionOption exception_handler (PedException* ex)
{
    /* Ignore warnings and informational exceptions */
    if (ex->type < PED_EXCEPTION_ERROR)
    {
	/* TODO: Indicate that it's only a warning */
	autopartkit_error(0, ex->message);
	return PED_EXCEPTION_IGNORE;
    }
    autopartkit_error(0, ex->message);
    /* Really don't know what a good default is here */
    return PED_EXCEPTION_CANCEL;
}

int main (int argc, char *argv[])
{
    PedDevice *dev = NULL;
    diskspace_req_t *disk_reqs = NULL;
    const char *tablefile = NULL;
    int retval = 1;

    client = debconfclient_new ();

    autopartkit_log(1, "Using '%s' default disk label type\n",
		    default_disk_label());

#if defined(LVM_HACK)
    if (0 != lvm_init())
        autopartkit_log(1, "Unable to initialize LVM support.  "
			"Continuing anyway.\n");
#endif /* LVM_HACK */

    disable_kmsg(1);
    ped_exception_set_handler(exception_handler);
    PED_INIT();
    disable_kmsg(0);

    if (argc > 1)
#if defined(TEST)
        tablefile = "default.table";
#else
        tablefile = argv[1];
#endif /* TEST */

    if ( ! tablefile )
        autopartkit_error(1, "usage: %s <table file>\n", argv[0]);

    disk_reqs = load_partitions(tablefile);

    /* Step 1 & 2 : discover & choose device */
    dev = choose_device();
    if (! dev)
    {
        autopartkit_error(0, "Unable to find any hard drives to partition!");
        goto end;
    }

    if (NULL == disk_reqs)
    {
        autopartkit_error(0, "Unable to load partition table '%s'.",
                          tablefile);
	goto end;
    }
    else
    {
        autopartkit_log(2, "pre_confirm %d %d\n", 0,0);
        autopartkit_confirm();
        autopartkit_log(2, "post_confirm %d %d\n", 0,0);

        nuke_all_partitions();
        make_partitions(disk_reqs, dev);

	free_partition_list(disk_reqs);
    }

    retval = 0;

end:
    PED_DONE();

    debconfclient_delete(client);
    return retval;
}
