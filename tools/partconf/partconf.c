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

#include <cdebconf/debconfclient.h>

#include "partconf.h"

static struct debconfclient *debconf = NULL;
struct partition *parts[MAX_PARTS];
static char *filesystems[MAX_FSES];
static int part_count = 0, fs_count = 0;
static char *fschoices;

static PedExceptionOption
my_exception_handler(PedException* ex)
{
    if (ex->type < PED_EXCEPTION_ERROR) {
        return PED_EXCEPTION_IGNORE;
    }
    return PED_EXCEPTION_CANCEL;
}

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
    if ((fp = fopen("/proc/mdstats", "r")) == NULL)
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
            break;
        }
    }
    fclose(fp);
}

int
get_all_partitions(void)
{
    char buf[1024], *ptr, partname[1024], tmp[1024];
    FILE *fp, *fptmp;
    DIR *d;
    struct dirent *dent;
    char *discs[MAX_DISCS];
    struct partition *p;
    int disc_count = 0;
    int i, cont, size;

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
            discs[disc_count] = strdup(buf);
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
        if (part_count >= MAX_PARTS)
            break;
        p = malloc(sizeof(*p));
        p->path = strdup(partname);
        p->fstype = NULL;
        p->fsid = NULL;
        p->op.filesystem = NULL;
        p->op.mountpoint = NULL;
        p->op.done = 0;
        test_lvm(p);
        test_evms(p);
        test_raid(p);
        // FIXME: Other tests?
        parts[part_count++] = p;
    }
    // Add partitions from all the disks we found
    for (i = 0; i < disc_count; i++) {
        PedDevice *dev;
        PedDisk *disk;
        PedPartition *part = NULL;
        const PedFileSystemType *fstype;
        char *foo;

        asprintf(&foo, "%s/disc", discs[i]);
        if ((dev = ped_device_get(foo)) == NULL) {
            continue;
        }
        if ((disk = ped_disk_new(dev)) == NULL) {
            continue;
        }
        while ((part = ped_disk_next_partition(disk, part)) != NULL) {
            if (part->type & (PED_PARTITION_METADATA | PED_PARTITION_FREESPACE | PED_PARTITION_EXTENDED))
                continue;
            if (ped_partition_is_flag_available(part, PED_PARTITION_LVM) &&
                    ped_partition_get_flag(part, PED_PARTITION_LVM)) {
                continue;
            }
            if (ped_partition_is_flag_available(part, PED_PARTITION_RAID) &&
                    ped_partition_get_flag(part, PED_PARTITION_RAID)) {
                continue;
            }
            p = malloc(sizeof(*p));
            p->path = ped_partition_get_path(part);
            fstype = part->fs_type;
            if (fstype != NULL)
                p->fstype = strdup(fstype->name);
            else
                p->fstype = NULL;
            p->fsid = NULL;
            p->size = PART_SIZE_BYTES(dev, part);
            p->op.filesystem = NULL;
            p->op.mountpoint = NULL;
            p->op.done = 0;
            parts[part_count++] = p;
        }
    }
    return part_count;
}

char *
build_part_choices(struct partition *parts[], const int part_count)
{
    char *list[part_count];
    char *tmp, *tmp2;
    int i, max_len;

    //printf("part_count=%d\n", part_count);
    if (part_count <= 0)
        return NULL;
    max_len = 0;
    for (i = 0; i < part_count; i++) {
        if (strlen(parts[i]->path) > max_len)
            max_len = strlen(parts[i]->path);
    }
    // pad with spaces
    for (i = 0; i < part_count; i++) {
        asprintf(&list[i], "%-*s", max_len, parts[i]->path);
    }
    max_len = strlen("n/a");
    for (i = 0; i < part_count; i++) {
        if (parts[i]->size > 0 && strlen(size_desc(parts[i]->size)) > max_len)
            max_len = strlen(size_desc(parts[i]->size));
    }
    // add and pad
    for (i = 0; i < part_count; i++) {
        asprintf(&tmp, "%s  %-*s", list[i], max_len,
                (parts[i]->size > 0) ? size_desc(parts[i]->size) : "n/a");
        free(list[i]);
        list[i] = tmp;
    }
    max_len = strlen("n/a");
    for (i = 0; i < part_count; i++) {
        if (parts[i]->fstype != NULL && strlen(parts[i]->fstype) > max_len)
            max_len = strlen(parts[i]->fstype);
    }
    for (i = 0; i < part_count; i++) {
        asprintf(&tmp, "%s  %-*s", list[i], max_len,
                (parts[i]->fstype != NULL) ? parts[i]->fstype : "n/a");
        free(list[i]);
        list[i] = tmp;
    }
    max_len = 0;
    for (i = 0; i < part_count; i++) {
        if (parts[i]->op.filesystem != NULL && strlen(parts[i]->op.filesystem) > max_len)
            max_len = strlen(parts[i]->op.filesystem);
    }
    if (max_len > 0)
        for (i = 0; i < part_count; i++) {
            asprintf(&tmp, "%s  %-*s", list[i], max_len,
                    (parts[i]->op.filesystem != NULL) ? parts[i]->op.filesystem : "");
            free(list[i]);
            list[i] = tmp;
        }
    max_len = 0;
    for (i = 0; i < part_count; i++) {
        if (parts[i]->op.mountpoint != NULL && strlen(parts[i]->op.mountpoint) > max_len)
            max_len = strlen(parts[i]->op.mountpoint);
    }
    if (max_len > 0)
        for (i = 0; i < part_count; i++) {
            asprintf(&tmp, "%s  %-*s", list[i], max_len,
                    (parts[i]->op.mountpoint != NULL) ? parts[i]->op.mountpoint : "");
            free(list[i]);
            list[i] = tmp;
        }
    /* PHEW */
    tmp = list[0];
    //printf("<%s>\n", list[0]);
    for (i = 1; i < part_count; i++) {
        //printf("<%s>\n", list[i]);
        asprintf(&tmp2, "%s, %s", tmp, list[i]);
        free(list[i]);
        free(tmp);
        tmp = tmp2;
    }
    return tmp;
}

static char *
fs_to_choice(char *fs)
{
    static char *choicefmt = NULL;
    char *tmp;

    // Cache the format string
    if (choicefmt == NULL) {
        debconf->command(debconf, "METAGET",
                "partconf/internal-create-fs-choice", "description", NULL);
        choicefmt = strdup(debconf->value);
    }
    asprintf(&tmp, choicefmt, fs);
    return tmp;
}

char *
build_fs_choices(void)
{
    char *tmp, *tmp2;
    int i;

    tmp = NULL;
    for (i = 0; i < fs_count; i++) {
        if (tmp == NULL)
            tmp2 = fs_to_choice(filesystems[i]);
        else
            asprintf(&tmp2, "%s, %s", tmp, fs_to_choice(filesystems[i]));
        free(tmp);
        tmp = tmp2;
    }
    return tmp;
}

// Check /proc/filesystems and /sbin/mkfs.* to figure out which
// filesystems are available.
int
get_all_filesystems(void)
{
    FILE *fp, *fptmp;
    char buf[1024], buf2[1024], *ptr;

    fp = fopen("/proc/filesystems", "r");
    if (fp == NULL)
        return 0;
    fs_count = 0;
    while ((ptr = fgets(buf, sizeof(buf), fp)) != NULL) {
        if (strstr(ptr, "nodev") == ptr)
            continue;
        while (*ptr != '\0' && isspace(*ptr))
            ptr++;
        if (*ptr == '\0')
            continue;
        filesystems[fs_count] = ptr;
        while (*ptr != '\0' && !isspace(*ptr) && *ptr != '\n')
            ptr++;
        *ptr = '\0';
        // Check if there's a corresponding mkfs program
        snprintf(buf2, sizeof(buf2)-1, "/sbin/mkfs.%s", filesystems[fs_count]);
        if ((fptmp = fopen(buf2, "r")) == NULL)
            continue;
        fclose(fptmp);
        filesystems[fs_count] = strdup(filesystems[fs_count]);
        fs_count++;
    }
    fclose(fp);
    return fs_count;
}

static int
sanity_checks(void)
{
    int i, j, ok;
    static char *bad_mounts[] = {
        "/proc",
        "/dev",
        "/etc",
        NULL
    };

    // Check for root file system
    if (!check_proc_mounts("")) {
        ok = 0;
        for (i = 0; i < part_count; i++) {
            if (parts[i]->op.mountpoint == NULL)
                continue;
            if (strcmp(parts[i]->op.mountpoint, "/") == 0) {
                ok = 1;
                break;
            }
        }
        if (!ok) {
            debconf->command(debconf, "INPUT critical", "partconf/sanity-no-root", NULL);
            debconf->command(debconf, "GO", NULL);
            return 0;
        }
    }
    // Check for bad mount points (/proc, /dev, /etc)
    ok = 1;
    for (i = 0; i < part_count; i++) {
        if (parts[i]->op.mountpoint == NULL)
            continue;
        for (j = 0; bad_mounts[j] != NULL; j++) {
            if (strcmp(parts[i]->op.mountpoint, bad_mounts[j]) == 0) {
                debconf->command(debconf, "SUBST", "partconf/sanity-bad-mount",
                        "MOUNT", parts[i]->op.mountpoint, NULL);
                debconf->command(debconf, "INPUT critical", "partconf/sanity-bad-mount", NULL);
                debconf->command(debconf, "GO", NULL);
                ok = 0;
                break;
            }
        }
    }
    if (!ok)
        return 0;
    // Check for double mounts
    ok = 1;
    for (i = 0; i < part_count; i++) {
        if (parts[i]->op.mountpoint == NULL)
            continue;
        if (check_proc_mounts(parts[i]->op.mountpoint)) {
            debconf->command(debconf, "SUBST", "partconf/sanity-double-mount",
                    "MOUNT", parts[i]->op.mountpoint, NULL);
            debconf->command(debconf, "INPUT critical", "partconf/sanity-double-mount", NULL);
            debconf->command(debconf, "GO", NULL);
            ok = 0;
        }
        else
            for (j = i+1; j < part_count; j++) {
                if (parts[j]->op.mountpoint == NULL)
                    continue;
                if (strcmp(parts[i]->op.mountpoint, parts[j]->op.mountpoint) == 0) {
                    debconf->command(debconf, "SUBST", "partconf/sanity-double-mount",
                            "MOUNT", parts[i]->op.mountpoint, NULL);
                    debconf->command(debconf, "INPUT critical",
                            "partconf/sanity-double-mount", NULL);
                    debconf->command(debconf, "GO", NULL);
                    ok = 0;
                    break;
                }
            }
    }
    if (!ok)
        return 0;
    // Confirm
    debconf->command(debconf, "SET", "partconf/confirm", "false", NULL);
    debconf->command(debconf, "INPUT critical", "partconf/confirm", NULL);
    if (debconf->command(debconf, "GO", NULL) == 30)
        return 0;
    debconf->command(debconf, "GET", "partconf/confirm", NULL);
    if (strcmp(debconf->value, "false") == 0)
        return 0;
    return 1;
}

static int
mountpoint_sort_func(const void *v1, const void *v2)
{
    struct partition *p1, *p2;

    p1 = *(struct partition **)v1;
    p2 = *(struct partition **)v2;
    if (strstr(p1->path, p2->path) == p1->path)
        return 1;
    else if (strstr(p2->path, p1->path) == p2->path)
        return -1;
    else
        return strcmp(p1->path, p2->path);
}

/*
 * Like mkdir -p
 */
static void
makedirs(const char *dir)
{
    DIR *d;
    char *dirtmp, *basedir;

    if ((d = opendir(dir)) != NULL) {
        closedir(d);
        return;
    }
    if (mkdir(dir, 0755) < 0) {
        dirtmp = strdup(dir);
        basedir = dirname(dirtmp);
        makedirs(basedir);
        free(dirtmp);
    }
}

static void
finish(void)
{
    int i, ret;
    char *cmd, *mntpt, *errq = NULL;

    qsort(parts, part_count, sizeof(struct partition *), mountpoint_sort_func);
    for (i = 0; i < part_count; i++) {
        if (parts[i]->op.filesystem == NULL)
            continue;
        if (strcmp(parts[i]->op.filesystem, "swap") == 0) {
            asprintf(&cmd, "mkswap %s >/dev/null 2>>/var/log/messages", parts[i]->path);
            ret = system(cmd);
            free(cmd);
            if (ret != 0) {
                errq = "partconf/failed-mkswap";
                break;
            }
            asprintf(&cmd, "swapon %s >/dev/null 2>>/var/log/messages", parts[i]->path);
            ret = system(cmd);
            free(cmd);
            if (ret != 0) {
                errq = "partconf/failed-swapon";
                break;
            }
        } else {
            asprintf(&cmd, "mkfs.%s %s >/dev/null 2>>/var/log/messages",
                    parts[i]->op.filesystem, parts[i]->path);
            ret = system(cmd);
            free(cmd);
            if (ret != 0) {
                errq = "partconf/failed-mkfs";
                debconf->command(debconf, "SUBST", errq, "FS", parts[i]->op.filesystem, NULL);
                break;
            }
            if (parts[i]->op.mountpoint == NULL)
                continue;
            asprintf(&mntpt, "/target%s", parts[i]->op.mountpoint);
            makedirs(mntpt);
            ret = mount(parts[i]->path, mntpt, parts[i]->op.filesystem, 0xC0ED0000, NULL);
            // Ignore failure due to unknown filesystem
            if (ret < 0 && errno != ENODEV) {
                append_message("mount: %s\n", strerror(errno));
                errq = "partconf/failed-mount";
                debconf->command(debconf, "SUBST", errq, "MOUNT", mntpt, NULL);
                free(mntpt);
                break;
            }
            free(mntpt);
            //asprintf(&cmd, "mount %s -t%s %s >/dev/null 2>>/var/log/messages",
            //        parts[i]->path, parts[i]->op.filesystem, parts[i]->op.mountpoint);
            //ret = system(cmd);
            //free(cmd);
        }
    }
    if (errq != NULL) {
        debconf->command(debconf, "SUBST", errq, "PARTITION", parts[i]->path, NULL);
        debconf->command(debconf, "INPUT critical", errq, NULL);
        debconf->command(debconf, "GO", NULL);
        exit(30);
    }
}

static struct partition *curr_part = NULL;
static char *curr_q = NULL;

static int
partition_menu(void)
{
    char *choices;
    
    /* Get partition information */
    choices = build_part_choices(parts, part_count);
    //printf("Choices: <%s>\n", choices);
    debconf->command(debconf, "SUBST", "partconf/partitions", "PARTITIONS", choices, NULL);
    free(choices);
    debconf->command(debconf, "INPUT critical", "partconf/partitions", NULL);
    curr_part = NULL;
    return 0;
}

static int
filesystem(void)
{
    int i;
    char *partname, *ptr;

    debconf->command(debconf, "GET", "partconf/partitions", NULL);
    if (strcmp(debconf->value, "Finish") == 0) {
        if (!sanity_checks())
            return 1; // will back up
        finish();
    }
    if (strcmp(debconf->value, "Abort") == 0)
        return -1;
    partname = strdup(debconf->value);
    if ((ptr = strchr(partname, ' ')) == NULL)
        return -1;
    *ptr = '\0';
    curr_part = NULL;
    for (i = 0; i < part_count; i++)
        if (strcmp(parts[i]->path, partname) == 0) {
            curr_part = parts[i];
            break;
        }
    if (curr_part == NULL)
        return -1;
    if (curr_part->fstype != NULL) {
        curr_q = "partconf/existing-filesystem";
        debconf->command(debconf, "SUBST", curr_q, "FSTYPE", curr_part->fstype, NULL);
    } else
        curr_q = "partconf/create-filesystem";
    debconf->command(debconf, "SUBST", curr_q, "FSCHOICES", fschoices, NULL);
    debconf->command(debconf, "SUBST", curr_q, "PARTITION", curr_part->path, NULL);
    debconf->command(debconf, "INPUT critical", curr_q, NULL);
    return 0;
}

static int
mountpoint(void)
{
    int i;

    debconf->command(debconf, "GET", curr_q, NULL);
    if (strcmp(debconf->value, "Leave the file system intact") == 0) {
        // Do nothing
    }
    else if (strcmp(debconf->value, "Create swap space") == 0) {
        free(curr_part->op.filesystem);
        curr_part->op.filesystem = strdup("swap"); // special case
    } else {
        char *tmp = strdup(debconf->value);

        for (i = 0; i < fs_count; i++) {
            if (strcmp(tmp, fs_to_choice(filesystems[i])) == 0)
                break;
        }
        free(tmp);
        if (i == fs_count)
            return -1;
        free(curr_part->op.filesystem);
        curr_part->op.filesystem = strdup(filesystems[i]);
    }
    if (curr_part->op.filesystem != NULL && strcmp(curr_part->op.filesystem, "swap") != 0) {
        // TODO: default to current mount point, if any
        debconf->command(debconf, "SUBST", "partconf/mountpoint", "PARTITION",
                curr_part->path, NULL);
        debconf->command(debconf, "INPUT critical", "partconf/mountpoint", NULL);
    }
    return 0;
}

static int do_mount_manual = 0;

static int
mountpoint_manual(void)
{
    do_mount_manual = 0;
    if (curr_part->op.filesystem == NULL || strcmp(curr_part->op.filesystem, "swap") == 0)
        return 0;
    debconf->command(debconf, "GET", "partconf/mountpoint", NULL);
    if (strcmp(debconf->value, "Don't mount it") == 0) {
        free(curr_part->op.mountpoint);
        curr_part->op.mountpoint = NULL;
    } else if (strcmp(debconf->value, "Enter manually") == 0) {
        debconf->command(debconf, "SUBST", "partconf/mountpoint-manual", "PARTITION",
                curr_part->path, NULL);
	debconf->command(debconf, "INPUT critical", "partconf/mountpoint-manual", NULL);
	do_mount_manual = 1;
    } else {
	//printf("Setting mountpoint to %s\n", debconf->value);
        free(curr_part->op.mountpoint);
        curr_part->op.mountpoint = strdup(debconf->value);
    }
    return 0;
}

static int
fixup(void)
{
    if (!do_mount_manual)
	return 0;
    debconf->command(debconf, "GET", "partconf/mountpoint-manual", NULL);
    free(curr_part->op.mountpoint);
    curr_part->op.mountpoint = strdup(debconf->value);
    return 0;
}

int
main(int argc, char *argv[])
{
    int state = 0, ret;
    int (*states[])() = {
        partition_menu,
        filesystem,
        mountpoint,
        mountpoint_manual,
        fixup, // never does an INPUT, just handles the manual mountpoint result
        NULL
    };
    debconf = debconfclient_new();
    debconf->command(debconf, "CAPB", "backup", NULL);
    ped_exception_set_handler(my_exception_handler);
    modprobe("ext3 reiserfs jfs xfs"); // FIXME: Any others?
    modprobe("lvm-mod");
    // FIXME: EVMS?
    modprobe("raid0 raid1 raid5 linear");
    if (get_all_partitions() <= 0) {
        debconf->command(debconf, "INPUT critical", "partconf/no-partitions", NULL);
        debconf->command(debconf, "GO", NULL);
        return 1;
    }
    if (get_all_filesystems() <= 0) {
        debconf->command(debconf, "INPUT critical", "partconf/no-filesystems", NULL);
        debconf->command(debconf, "GO", NULL);
        return 1;
    }
    fschoices = build_fs_choices();
    while (state >= 0) {
        ret = states[state]();
        if (ret < 0)
            state = -1;
        else if (ret == 0 && debconf->command(debconf, "GO", NULL) == 0)
            state++;
        else
            state--;
        if (state >= 0 && states[state] == NULL)
            state = 0;
    }
    if (state < 0)
        ret = 30;
    return ret;
}
