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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   
*/

/*
 * Here's the global algorithm used by autopartkit :
 *
 * 1. Detect available disks.
 * 
 * 2. Choose a disk :
 *    - automatic if there's only a single disk
 *    - from the list of available disk if more than one disk is available
 *      Provide enough info on the disks to do an informed choice :
 *       Device   | Model         | Size | NbPart | FreeSpace | Space in FAT
 *       -------------------------------------------------------------------
 *       /dev/hda | IBM...        | 30G  | 1      | 0M        | 25G
 *       /dev/hdb | Maxtor...     | 12G  | 6      | 5G        | 3G
 *    - by asking the device name to the user if no disk was found
 *
 *    (if a windows bootable partition is found during the scan, the
 *    parition device is written in /etc/windows_part)
 *    
 * 3. From now on, we work only on the selected disk.
 *    If the disk has no partition table, create an empty one
 *    (the only "virtual" partition should be a big FREE_SPACE partition)
 *    
 * 4. Check how much space is available in a contiguous block. If
 *    enough space is available, go on 6.
 *    
 * 5. If free space is missing, check how much place can be made by
 *    resizing FAT partitions (and FAT partition are resized in such a way
 *    that they always have at least 25% free space). If enough space can
 *    be recovered then resize partition (move them arround) and go on,
 *    otherwise fail and ask to manually partition.
 *    [ special steps may be needed with extended partitions ]
 *    
 * 6. Create the required partitions in the free space, mount them
 *    on /target and write the entries in /target/etc/fstab.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <parted/parted.h>
#include <cdebconf/debconfclient.h>

/* Required or not ?
#include <linux/fs.h>
#include <asm/page.h> 
 */
 
#include <sys/mount.h>
#include <sys/swap.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define MEGABYTE (1024 * 1024)
#define _(ARG) ARG
#define MINSIZE_FACTOR 1.33
 
/* Need to define on a per arch basis */
#define DISK_LABEL "msdos"

/* Write /etc/windows_part?
#define WRITE_WINDOWS_PART
*/

/* Ignore devfs devices */
#define IGNORE_DEVFS_DEVICES
 
/* Partitions sizes */
#define SIZE_ROOT 128
#define SIZE_SWAP 256
#define SIZE_TEMP 64
#define SIZE_VAR  512
#define MIN_SIZE_USR 1536
#define MIN_SIZE_HOME 128
#define MIN_SIZE_GLOBAL (SIZE_ROOT + SIZE_SWAP + SIZE_TEMP + SIZE_VAR + MIN_SIZE_USR + MIN_SIZE_HOME)
 
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
void autopartkit_error(int, char*);
void debconf_debug(char*, char*);
char* debconf_input(char*, char*);
PedDevice* choose_device();
DeviceStats* get_device_stats(PedDevice*);

void debconf_debug (char* variable, char* value)
{
    client->command(client, "FSET", "autopartkit/debug", "seen", "false", NULL);
    client->command(client, "SUBST", "autopartkit/debug", 
		    "variable", variable, NULL);
    client->command(client, "SUBST", "autopartkit/debug",
		    "value", value, NULL);
    client->command(client, "INPUT high", "autopartkit/debug", NULL);
    client->command(client, "GO", NULL);
}

void autopartkit_error (int rv, char * msg)
{
    client->command(client, "FSET", "autopartkit/error", "seen", "false", NULL);
    client->command(client, "SUBST", "autopartkit/error", "error",
			    msg, NULL);
    client->command(client, "INPUT high", "autopartkit/error", NULL);
    client->command(client, "GO", NULL);
    if (rv)
        exit (rv);
}

char * debconf_input (char *priority, char *template)
{
    client->command (client, "FSET", template, "seen", "false", NULL);
    client->command (client, "INPUT", priority, template, NULL);
    client->command (client, "GO", NULL);
    client->command (client, "GET", template, NULL);
    return client->value;
}

void debconf_note(char * template)
{
    client->command(client, "FSET", template, "seen", "false", NULL);
    client->command(client, "INPUT high", template, NULL);
    client->command(client, "GO", NULL);
}

int debconf_bool(char *priority, char *template)
{
    char *value;

    value = debconf_input(priority, template);
    if (strstr(value, "true"))
	return 1;
    if (strstr(value, "false"))
	return 0;
    debconf_debug("unknown bool value", value);
    return 0;
}

void autopartkit_log(char * format, int begin, int end)
{
    FILE* log;
    log = fopen("/var/log/autopartkit.log", "a");
    fprintf(log, format, begin, end);
    fclose(log);
}

void autopartkit_confirm()
{
    static int confirm = 0;
    if (confirm)
	return;
    if (debconf_bool("high", "autopartkit/confirm"))
    {
	confirm = 1;
	return;
    } else {
	exit(1);
    }
}

void disable_kmsg(int disable)
{
    static char level[64];
    FILE* printk;
    if (disable)
    {
	int read;
	printk = fopen("/proc/sys/kernel/printk", "r");
	if (! printk)
	    autopartkit_error(0, "Could not open /proc/sys/kernel/printk");
	read = fread(level, 1, 63, printk);
	level[read] = '\0';
	fclose(printk);
	printk = fopen("/proc/sys/kernel/printk", "w");
	if (! printk)
	    autopartkit_error(0, "Could not open /proc/sys/kernel/printk");
	fprintf(printk, "0\n");
	fclose(printk);
    } else {
	printk = fopen("/proc/sys/kernel/printk", "w");
	if (! printk)
	    autopartkit_error(0, "Could not open /proc/sys/kernel/printk");
	fprintf(printk, "%s", level);
	fclose(printk);
    }
}

/* 
 * Step 1 & 2: Discover and select the device
 *
 * Probe the available device. Automatically select it, if only one is
 * available. Ask user input if none is detected. Otherwise provide a list
 * to choose from.
 * 
 */

/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
#define TABLE_HEADER _("Device    Model               Size   Free   FreeFat NbPart")
#define TABLE_SIZE 2048
#define LIST_SIZE 512
PedDevice* choose_device()
{
    PedDevice *dev = NULL;
    char      *dev_name = NULL;
    int	       nb_try = 0;
    char       device_list[LIST_SIZE], default_device[128], table[TABLE_SIZE];
    char      *ptr_list, *ptr_table;
    DeviceStats *stats;
    
    disable_kmsg(1);
    ped_device_probe_all();
    disable_kmsg(0);

    dev = ped_device_get_next(NULL);

    if (dev == NULL)
    {
	/* No devices detected */
	while (dev == NULL) 
	{
	    dev_name = debconf_input("critical", "autopartkit/device_name");
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
	client->command(client, "FSET", "autopartkit/choose_device", "seen", 
			"false", NULL);
	client->command(client, "SUBST", "autopartkit/choose_device", 
			"CHOICES", device_list, NULL);
	client->command(client, "SUBST", "autopartkit/choose_device", 
			"DEFAULT", default_device, NULL);
	client->command(client, "SUBST", "autopartkit/choose_device", 
			"TABLEHEADER", TABLE_HEADER, NULL);
	client->command(client, "SUBST", "autopartkit/choose_device", 
			"TABLE", table, NULL);
	client->command(client, "INPUT critical", "autopartkit/choose_device", 
			NULL);
        client->command(client, "GO", NULL);
	client->command(client, "GET", "autopartkit/choose_device", NULL);
	disable_kmsg(1);
	dev = ped_device_get(client->value);
	disable_kmsg(0);
	if (++nb_try > 3)
	    break;
    }
    return dev;
}

/*
 * Returns a malloced block of stats concerning the indicated device.
 *
 */
DeviceStats* get_device_stats(PedDevice* dev)
{
    DeviceStats       *stats;
    PedDisk	      *disk;
    PedPartition      *part;
    const PedFileSystemType *fs_type;
    PedFileSystem     *fs;
    PedConstraint     *constraint;
    FILE	      *winpart;
    PedPartitionFlag  boot_flag = ped_partition_flag_get_by_name("boot");

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
    disk = ped_disk_open(dev);
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
	    PedSector space = (PedSector)((part->geom.length * dev->sector_size)
					/ MEGABYTE);
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
			    MINSIZE_FACTOR))
		{
		    stats->free_space_in_fat += ((part->geom.length -
			(PedSector) (constraint->min_size * MINSIZE_FACTOR)) 
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
#endif
	    }
	}
    }
    ped_disk_close(disk);
    return stats;
}

/* 
 * Step 3 : open the disk and create a partition table if needed
 *
 */
PedDisk* open_disk(PedDevice* dev)
{
    PedDisk* disk;
    disk = ped_disk_open(dev);

    if (disk == NULL)
    {
	/* Need to create a partition table */
	disk = ped_disk_create(dev, ped_disk_type_get(DISK_LABEL));
    }
    return disk;
}

/*
 * Resize all FAT partitions in order to free space.
 *
 */
int resize_fat_partitions(PedDisk *disk)
{
    PedPartition *part;
    PedFileSystem *fs;
    PedFileSystemType *fs_type = NULL;
    PedConstraint *constraint;

    /* Iterate over all FAT partitions and resize them */
    for(part = ped_disk_next_partition(disk, NULL); part;
	part = ped_disk_next_partition(disk, part))
    {
	if (part->type != PED_PARTITION_PRIMARY &&
	    part->type != PED_PARTITION_LOGICAL)
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

	    if ((! ped_disk_set_partition_geom(disk, part, constraint,
		     part->geom.start, part->geom.start + 
		     (PedSector)(constraint->min_size * MINSIZE_FACTOR)))
	        ||
		(! ped_file_system_resize(fs, &part->geom))
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
    ped_disk_write(disk);
    return 1;
}

/*
 * Create all the partitions on the disk
 *
 */
void create_partitions(PedDisk *disk)
{
    PedPartition *free = NULL, *new = NULL, *part = NULL;
    PedConstraint *any;
    PedFileSystemType *fs_type_ext2, *fs_type_swap;
    int mount_points[6]; /* 0->/ 1->swap 2->/tmp 3->/var 4->/usr 5->/home 
			  * value is partition number
			  */
    char device[64];
    FILE *fstab;
    char *diskpath = strdup(disk->dev->path);
    PedSector start, end;
    
    /* Look for empty space on the disk */
    for(part = ped_disk_next_partition(disk, NULL); part;
	part = ped_disk_next_partition(disk, part))
    {
	if (part->type == PED_PARTITION_FREESPACE)
	{
	    if (((part->geom.length * disk->dev->sector_size) / MEGABYTE)
		    > MIN_SIZE_GLOBAL)
	    {
		free = part;
		break;
	    }
	}
    }
    if (! free)
    {
	autopartkit_error(1, "Not enough contiguous free space");
    }
    
    fs_type_ext2 = ped_file_system_type_get("ext2");
    fs_type_swap = ped_file_system_type_get("linux-swap");
    
    autopartkit_log("Creating parts on disk having sectors %d - %d\n", 
		    0, disk->dev->length);
    /* Create a big extended partition in where i'll put all other parts */
    any = ped_constraint_any(disk);
    autopartkit_log("Trying to create extented part on %d - %d\n", 
		    free->geom.start, free->geom.end);
    free = ped_partition_new(disk, PED_PARTITION_EXTENDED, NULL,
			    free->geom.start, free->geom.end);
    ped_disk_add_partition(disk, free, any);
    autopartkit_log("Created extented part on %d - %d\n", 
		    free->geom.start, free->geom.end);
    ped_constraint_destroy(any);
    
    new = free ;
    /* Create / */
    start = new->geom.start + 1;
    end = new->geom.start + 1 + (SIZE_ROOT * MEGABYTE) / disk->dev->sector_size;
    any = ped_constraint_any(disk);
    autopartkit_log("Trying to create root (/) part on %d - %d\n", start, end);
    new = ped_partition_new(disk, PED_PARTITION_LOGICAL,
			    fs_type_ext2, start, end);
    ped_disk_add_partition(disk, new, any);
    ped_file_system_close(ped_file_system_create(&new->geom, fs_type_ext2));
    autopartkit_log("Created root (/) part on %d - %d\n", 
		    new->geom.start, new->geom.end);
    ped_constraint_destroy(any);
    mount_points[0] = new->num;

    /* Create swap */
    start = new->geom.end + 1;
    end = start + (SIZE_SWAP * MEGABYTE) / disk->dev->sector_size;
    any = ped_constraint_any(disk);
    autopartkit_log("Trying to create swap part on %d - %d\n", start, end);
    new = ped_partition_new(disk, PED_PARTITION_LOGICAL,
			    fs_type_swap, start, end);
    ped_disk_add_partition(disk, new, any);
    ped_file_system_close(ped_file_system_create(&new->geom, fs_type_swap));
    autopartkit_log("Created swap part on %d - %d\n", 
		    new->geom.start, new->geom.end);
    ped_constraint_destroy(any);
    mount_points[1] = new->num;
   
    /* Create /tmp */
    start = new->geom.end + 1;
    end = start + (SIZE_TEMP * MEGABYTE) / disk->dev->sector_size;
    any = ped_constraint_any(disk);
    autopartkit_log("Trying to create /tmp part on %d - %d\n", start, end);
    new = ped_partition_new(disk, PED_PARTITION_LOGICAL,
			    fs_type_ext2, start, end);
    ped_disk_add_partition(disk, new, any);
    ped_file_system_close(ped_file_system_create(&new->geom, fs_type_ext2));
    autopartkit_log("Created /tmp part on %d - %d\n", 
		    new->geom.start, new->geom.end);
    ped_constraint_destroy(any);
    mount_points[2] = new->num;

    /* Create /var */
    start = new->geom.end + 1;
    end = start + (SIZE_VAR * MEGABYTE) / disk->dev->sector_size;
    any = ped_constraint_any(disk);
    autopartkit_log("Trying to create /var part on %d - %d\n", start, end);
    new = ped_partition_new(disk, PED_PARTITION_LOGICAL,
			    fs_type_ext2, start, end);
    ped_disk_add_partition(disk, new, any);
    ped_file_system_close(ped_file_system_create(&new->geom, fs_type_ext2));
    autopartkit_log("Created /var part on %d - %d\n", 
		    new->geom.start, new->geom.end);
    ped_constraint_destroy(any);
    mount_points[3] = new->num;
   
    /* Create /usr */
    start = new->geom.end + 1;
    end = start + (MIN_SIZE_USR * MEGABYTE) / disk->dev->sector_size + 
	  (PedSector)(0.20 * (free->geom.end -
	  (start + (MIN_SIZE_USR * MEGABYTE) / disk->dev->sector_size)));
    any = ped_constraint_any(disk);
    autopartkit_log("Trying to create /usr part on %d - %d\n", start, end);
    new = ped_partition_new(disk, PED_PARTITION_LOGICAL,
			    fs_type_ext2, start, end);
    ped_disk_add_partition(disk, new, any);
    ped_file_system_close(ped_file_system_create(&new->geom, fs_type_ext2));
    autopartkit_log("Created /usr part on %d - %d\n", 
		    new->geom.start, new->geom.end);
    ped_constraint_destroy(any);
    mount_points[4] = new->num;

    /* Create /home */
    start = new->geom.end + 1;
    end = free->geom.end;
    any = ped_constraint_any(disk);
    autopartkit_log("Trying to create /home part on %d - %d\n", start, end);
    new = ped_partition_new(disk, PED_PARTITION_LOGICAL,
			    fs_type_ext2, start, end);
    ped_disk_add_partition(disk, new, any);
    ped_file_system_close(ped_file_system_create(&new->geom, fs_type_ext2));
    autopartkit_log("Created /home part on %d - %d\n", 
		    new->geom.start, new->geom.end);
    ped_constraint_destroy(any);
    mount_points[5] = new->num;

    /* Finish the work */
    disable_kmsg(1);
    ped_disk_write(disk);
    ped_disk_close(disk);
    /* Close libparted, needs to do it here just to let the kernel
     * know of the new partition table so that mounts call actually work
     */
    ped_done();
    disable_kmsg(0);

    /* Mount partitions and write fstab */
    mkdir("/target", 0755);
    snprintf(device, 64, "%s%d", diskpath, mount_points[0]);
    if (mount(device, "/target", "ext2", MS_MGC_VAL, NULL) == -1)
	autopartkit_error(0, strerror(errno));

    mkdir("/target/etc", 0755);
    mkdir("/target/tmp", 0755);
    mkdir("/target/var", 0755);
    mkdir("/target/usr", 0755);
    mkdir("/target/home", 0755);
    mkdir("/target/floppy", 0755);
    mkdir("/target/cdrom", 0755);
    
    snprintf(device, 64, "%s%d", diskpath, mount_points[1]);
    disable_kmsg(1);
    swapon(device, 0);
    disable_kmsg(0);
    
    fstab = fopen("/target/etc/fstab.new", "w");

    fprintf(fstab, "# /etc/fstab: static file system information\n#\n");
    fprintf(fstab, "# <file system> <mount point> <type> <options> \t"
		   "<dump> <pass>\n");
    fprintf(fstab, "%s%d\t/\text2\tdefaults\t\t0\t1\n", diskpath,
		    mount_points[0]);
    fprintf(fstab, "%s%d\tnone\tswap\tsw\t\t0\t0\n", diskpath,
		    mount_points[1]);
    fprintf(fstab, "/proc\tproc\tproc\tdefaults\t\t0\t0\n");
    
    snprintf(device, 64, "%s%d", diskpath, mount_points[2]);
    if (mount(device, "/target/tmp", "ext2", MS_MGC_VAL, NULL) == -1)
	autopartkit_error(0, strerror(errno));
    fprintf(fstab, "%s\t/tmp\text2\tdefaults\t\t0\t2\n", device);

    snprintf(device, 64, "%s%d", diskpath, mount_points[3]);
    if (mount(device, "/target/var", "ext2", MS_MGC_VAL, NULL) == -1)
	autopartkit_error(0, strerror(errno));
    fprintf(fstab, "%s\t/var\text2\tdefaults\t\t0\t2\n", device);
    
    snprintf(device, 64, "%s%d", diskpath, mount_points[4]);
    if (mount(device, "/target/usr", "ext2", MS_MGC_VAL, NULL) == -1)
	autopartkit_error(0, strerror(errno));
    fprintf(fstab, "%s\t/usr\text2\tdefaults\t\t0\t2\n", device);
    
    snprintf(device, 64, "%s%d", diskpath, mount_points[5]);
    if (mount(device, "/target/home", "ext2", MS_MGC_VAL, NULL) == -1)
	autopartkit_error(0, strerror(errno));
    fprintf(fstab, "%s\t/home\text2\tdefaults\t\t0\t2\n", device);

    fprintf(fstab, "/dev/fd0\t/floppy\tauto\trw,user,noauto\t\t0\t0\n");
    fprintf(fstab, "/dev/cdrom\t/cdrom\tiso9660\tro,user,noauto\t\t0\t0\n");
    
    fclose(fstab);

    debconf_note("autopartkit/success");
}

/* 
 * This exception handler is required to not let libparted write warnings
 * on stdout and confuse cdebconf.
 *
 */
PedExceptionOption exception_handler (PedException* ex)
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
    PedDisk   *disk = NULL;
    DeviceStats *stats;
    
    client = debconfclient_new ();
    client->command (client, "TITLE", "Automatic Partitionner", NULL);

    disable_kmsg(1);
    ped_exception_set_handler(exception_handler);
    ped_init();
    disable_kmsg(0);
    /* Step 1 & 2 : discover & choose device */
    dev = choose_device();
    if (! dev)
	goto end;
    /* Step 3 & 4 : open the disk and create empty partition table if needed,
     *		    calculate some stats */
    stats = get_device_stats(dev);
    disk = open_disk(dev);
    if (! disk)
	goto end;
    /* Step 5 : decide if resizing is needed
     *
     *  / 128Mb
     *  swap 256Mb
     *  /tmp 64Mb
     *  /usr 1.5Gb mini
     *  /var 512Mb
     *  /home 200Mb mini (fill the rest)
     */ 
    if (stats->free_space < MIN_SIZE_GLOBAL)
    {
	/* Rather rough approach since the code is not very
	 * elaborated */
	if (stats->has_extended || stats->nb_part > stats->nb_fat_part
		|| stats->nb_fat_part > 2)
	{
	    /* Too difficult for the actual algorithm */
	    debconf_note("autopartkit/refuse");
	    goto endclose;
	} else {
	    /* We don't have any extended partition nor any
	     * non-fat partition */
	    if (stats->free_space_in_fat > MIN_SIZE_GLOBAL)
	    {
		/* No problem, just resize the fat partitions */
		autopartkit_confirm();
		if (! resize_fat_partitions(disk))
		{
		    goto endclose;
		}
	    } else {
		/* Well space is missing here */
		debconf_note("autopartkit/notenoughspace");
		goto endclose;
	    }
	}
    }
    /* Step 6 : create the partitions and close the parted stuff */
    autopartkit_confirm();
    create_partitions(disk);
    debconfclient_delete(client);
    return 0;
    
endclose:
    ped_disk_close(disk);
end:
    ped_done();

    debconfclient_delete(client);
    return 1;
}

