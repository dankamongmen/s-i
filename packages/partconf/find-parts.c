#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <ctype.h>
#include <libgen.h>
#include <errno.h>

#include "partconf.h"
#include <debian-installer.h>

// If it's an LVM volume, it's on the form
// /dev/<group>/<volume> and there's info in
// /proc/lvm/VGs/<group>/LVs/<volume>
static void
test_lvm(struct partition *p)
{
    FILE *fp;
    char buf[1024], name[1024];
    char *grp, *vol, *procfile;
    long long blocks = -1;

    if ((grp = strchr(p->path+1, '/')) == NULL)
        return;
    grp = strdup(grp+1);
    if ((vol = strchr(grp, '/')) == NULL)
        return;
    *vol++ = '\0';
    if (strchr(vol, '/') != NULL)
        return;
    asprintf(&procfile, "/proc/lvm/VGs/%s/LVs/%s", grp, vol);
    if ((fp = fopen(procfile, "r")) == NULL)
        return;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (strstr(buf, "name:") == buf)
            sscanf(buf, "name: %s", name);
        else if (strstr(buf, "size:") == buf)
            sscanf(buf, "size: %lld", &blocks);
    }
    fclose(fp);
    // So, *is* it right?
    if (strcmp(name, p->path) == 0 && blocks > 0)
        p->size = blocks * 512L;
}

// If it's an EVMS volume, it's on the form
// /dev/evms/<volume> and there's info in
// /proc/evms/volumes
// XXX THIS IS UNTESTED XXX
static void
test_evms(struct partition *p)
{
    FILE *fp;
    char buf[1024], name[1024];
    long long blocks;
    int i;

    if (strstr(p->path, "/dev/evms/") != p->path)
        return;
    if ((fp = fopen("/proc/evms/volumes", "r")) == NULL)
        return;
    // Skip three lines
    for (i = 0; i < 3; i++)
        if (fgets(buf, sizeof(buf), fp) == NULL)
            return;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        sscanf(buf, "%*d %*d %lld %*s %*s %s", &blocks, name);
        if (strcmp(p->path, name) == 0) {
            p->size = blocks * 512L;
            break;
        }
    }
    fclose(fp);
}

// RAID volumes are /dev/md/# for numbers #
// Stats are found in /dev/mdstats
// XXX THIS IS UNTESTED XXX
static void
test_raid(struct partition *p)
{
    FILE *fp;
    char buf[1024], name[1024];
    long long blocks;
    int volno;

    if (strstr(p->path, "/dev/md/") != p->path)
        return;
    if ((fp = fopen("/proc/mdstat", "r")) == NULL)
        return;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (strstr(buf, "md") != buf)
            continue;
        sscanf(buf, "md%d : ", &volno);
        sprintf(name, "/dev/md/%d", volno);
        if (strcmp(name, p->path) == 0) {
            if (fgets(buf, sizeof(buf), fp) == NULL)
                break;
            sscanf(buf, "%lld", &blocks);
            p->size = blocks * 512L;
            free(p->description);
            asprintf(&p->description, "RAID logical volume %d", volno);
            break;
        }
    }
    fclose(fp);
}

#ifndef FIND_PARTS_MAIN
int
block_partition(const char *part)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;

    dir = opendir(BLOCK_D);
    if(dir == NULL)
        return(0);

    while((entry = readdir(dir)) != NULL) {
        char *cmd = NULL;
        int ret;

        if(entry->d_name[0] == '.')
            continue;

        asprintf(&cmd, "/bin/sh %s/%s \"%s\" 1>/dev/null 2>&1",
            BLOCK_D, entry->d_name, part);
        ret = system(cmd);
        if(ret != 0) {
            closedir(dir);
            return(1);
        }
    }

    closedir(dir);
    return(0);
}
#endif

static void
get_partition_info(struct partition *p, PedPartition *part, PedDevice *dev, bool ignore_fs_type)
{
    if (PART_SIZE_BYTES(dev, part) > 0)
        p->size = PART_SIZE_BYTES(dev, part);
    if (!ignore_fs_type && part->fs_type != NULL) {
        if (strcmp(part->fs_type->name, "linux-swap") == 0)
            p->fstype = strdup("swap");
        else
            p->fstype = strdup(part->fs_type->name);
    } else {
        if (ped_partition_is_flag_available(part, PED_PARTITION_LVM) &&
                ped_partition_get_flag(part, PED_PARTITION_LVM)) {
            p->fstype = strdup("LVM");
        }
        if (ped_partition_is_flag_available(part, PED_PARTITION_RAID) &&
                ped_partition_get_flag(part, PED_PARTITION_RAID)) {
            p->fstype = strdup("RAID");
        }
    }
}

int
get_all_partitions(struct partition *parts[], const int max_parts, bool ignore_fs_type)
{
    char buf[1024], *ptr, partname[1024], tmp[1024];
    FILE *fp, *fptmp;
    DIR *d;
    struct dirent *dent;
    char *discs[MAX_DISCS];
    struct partition *p;
    int disc_count = 0, part_count = 0;
    int i, cont, size;
    PedDevice *dev;
    PedDisk *disk;
    PedPartition *part;

    if ((d = opendir("/dev/discs")) == NULL)
        return 0;
    while ((dent = readdir(d)) != NULL) {
        if (dent->d_name[0] == '.')
            continue;
        if (disc_count >= MAX_DISCS)
            break;
        snprintf(buf, sizeof(buf)-1, "/dev/discs/%s", dent->d_name);
        size = readlink(buf, tmp, sizeof(tmp)-1);
        if (size < 0) {
            snprintf(buf, sizeof(buf)-1, "/dev/discs/%s/disc", dent->d_name);
            size = readlink(buf, tmp, sizeof(tmp)-1);
            if (size < 0) {
                discs[disc_count] = strdup(buf);
            } else {
                tmp[size] = 0;
            // 2.2.x simulated devfs support 
                asprintf(&discs[disc_count], "/dev/%s", tmp);
            }
        } else {
            tmp[size] = 0;
            // Assumes the symlink starts with '../'
            asprintf(&discs[disc_count], "/dev/%s", tmp+3);
        }
        disc_count++;
    }
    if ((fp = fopen("/proc/partitions", "r")) == NULL) {
        perror("fopen(/proc/partitions)");
        return 0;
    }
    if (fgets(buf, sizeof(buf), fp) == NULL)
        return 0;
    if (fgets(buf, sizeof(buf), fp) == NULL)
        return 0;
    while ((ptr = fgets(buf, sizeof(buf), fp)) != NULL) {
        sscanf(buf, "%*d %*d %*d %s", tmp);
        strcpy(partname, "/dev/");
        strcat(partname, tmp);
        // Check if this is a disk or a partition on a known disk
        cont = 0;
        for (i = 0; i < disc_count; i++)
            if (strstr(partname, discs[i]) == partname) {
                cont = 1;
                break;
            }
        if (cont)
            continue;
        // Non-existent devices like 'hdc' (!) begone
        if ((fptmp = fopen(partname, "r")) == NULL)
            continue;
        fclose(fptmp);
        if (part_count >= max_parts)
            break;
        p = malloc(sizeof(*p));
        p->path = strdup(partname);
        p->description = strdup(p->path);
        p->fstype = NULL;
        p->fsid = NULL;
        p->size = 0L;
        p->op.filesystem = NULL;
        p->op.mountpoint = NULL;
        p->op.done = 0;
        test_lvm(p);
        test_evms(p);
        test_raid(p);
        // FIXME: Other tests?
        parts[part_count++] = p;
        // Open the partition/volume as if it was a disk, it should
        // just have one partition that we can toy with.
        if ((dev = ped_device_get(partname)) == NULL)
            continue;
        if ((disk = ped_disk_new(dev)) == NULL)
            continue;
        if ((part = ped_disk_next_partition(disk, NULL)) == NULL)
            continue;
        get_partition_info(p, part, dev, ignore_fs_type);
    }
    // Add partitions from all the disks we found
    for (i = 0; i < disc_count; i++) {
        char *foo;

        asprintf(&foo, "%s/disc", discs[i]);
        if ((dev = ped_device_get(foo)) == NULL) {
            free(foo);
            continue;
        }
        free(foo);
        if ((disk = ped_disk_new(dev)) == NULL) {
            continue;
        }
        part = NULL;
        while ((part = ped_disk_next_partition(disk, part)) != NULL) {
            if (part->type & (PED_PARTITION_METADATA | PED_PARTITION_FREESPACE | PED_PARTITION_EXTENDED))
                continue;

#ifndef FIND_PARTS_MAIN
            /* allow other udebs to block partitions */
            if(block_partition(ped_partition_get_path(part)) != 0)
                continue;
#endif

            p = malloc(sizeof(*p));
            p->path = ped_partition_get_path(part);
            if (strstr(p->path, "/dev/ide/") == p->path) {
                static char *targets[] = { "master", "slave" };
                int host, bus, target, lun, part;

                if (sscanf(p->path, "/dev/ide/host%d/bus%d/target%d/lun%d/part%d",
                            &host, &bus, &target, &lun, &part) == 5
                        && target >= 0 && target <= 1)
                    asprintf(&p->description, "IDE%d %s\\, part. %d",
                            2*host + bus + 1, targets[target], part);
                else
                    p->description = strdup(p->path);
            } else if (strstr(p->path, "/dev/scsi/") == p->path) {
                int host, bus, target, lun, part;

                if (sscanf(p->path, "/dev/scsi/host%d/bus%d/target%d/lun%d/part%d",
                            &host, &bus, &target, &lun, &part) == 5)
                    asprintf(&p->description, "SCSI%d (%d\\,%d\\,%d) part. %d",
                            host + 1, bus, target, lun, part);
                else
                    p->description = strdup(p->path);
            } else
                p->description = strdup(p->path);
            p->fstype = NULL;
            p->fsid = NULL;
            p->size = 0L;
            p->op.filesystem = NULL;
            p->op.mountpoint = NULL;
            p->op.done = 0;
            get_partition_info(p, part, dev, ignore_fs_type);
            parts[part_count++] = p;
        }
    }
    return part_count;
}


#ifdef FIND_PARTS_MAIN

int
main(int argc, char *argv[])
{
    struct partition *parts[MAX_PARTS];
    int part_count, i;
    bool ignore_fs_type = false;

    if (argc == 2 && strcmp(argv[1], "--ignore-fstype") == 0) {
        ignore_fs_type = true;
    }
    if ((part_count = get_all_partitions(parts, MAX_PARTS, ignore_fs_type)) <= 0)
        return 1;
    for (i = 0; i < part_count; i++) {
        printf("%s\t%s\t%s\n",
                parts[i]->path,
                parts[i]->fstype != NULL ? parts[i]->fstype : "",
                parts[i]->size > 0 ? size_desc(parts[i]->size) : "");
    }
    return 0;
}

#endif
