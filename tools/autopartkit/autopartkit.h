#ifndef AUTOPARTKIT_H
#define AUTOPARTKIT_H

#include <parted/device.h>  /* for PedSector */
#include <parted/disk.h>    /* for PedGeometry */

struct disk_info_t;

struct diskspace_req_s {
  char *mountpoint;
  char *fstype;
  int minsize; /* In MB */
  int maxsize; /* In MB */

  /* Used by the allocation algorithm in distribute.c */
  PedSector min_blk;
  PedSector max_blk;
  PedSector blocks;
  struct disk_info_t *curdisk;
};

typedef struct diskspace_req_s diskspace_req_t;

struct device_mntpoint_map_s {
  diskspace_req_t *mountpoint;
  char *devpath;
};

typedef struct device_mntpoint_map_s device_mntpoint_map_t;

struct disk_info_t {
    /* Ref to the disk/partition where there is free space */
    const char *path; /* Path to device */
    PedGeometry geom; /* Location of free space */

    PedSector capacity;
    PedSector freespace;
};

/* from distribute.c */
int distribute_partitions(struct disk_info_t diskinfo[],
			  struct diskspace_req_s reqs[]);
struct disk_info_t *get_free_space_list(void);
void print_list(struct disk_info_t diskinfo[], struct diskspace_req_s reqs[]);

/* from loadpartitions.c */
diskspace_req_t * load_partitions(const char *filename);
void free_partition_list(diskspace_req_t *list);
void list_dump(diskspace_req_t *list);

/* From choosetable.c */
const char *choose_profile_table(const char *profiles);

void autopartkit_log(const int level, const char * format, ...);
void autopartkit_error (int isfatal, const char * format, ...);

#define MEGABYTE (1024 * 1024)
/* Assumes 2048 byte sector_size */
#define MiB_TO_BLOCKS(mb) ((mb) * 2048l)
#define BLOCKS_TO_MiB(b)  ((b)  / 2048l)

/* from mapdevfs.c */
char *normalize_devfs(const char* path);

#endif /* AUTOPARTKIT_H */
