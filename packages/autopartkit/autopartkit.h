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

  int ondisk; /* flag if the file system should get a disk partition */

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
double get_ram_size(void);

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

/* from lvm.c */
int lvm_init(void);
int lvm_init_dev(const char *devpath);
int lvm_volumegroup_add_dev(const char *vgname, const char *devpath);
char *lvm_create_logicalvolume(const char *vgname, const char *lvname,
			       unsigned int mbsize);
int lvm_split_fstype(const char *str, int separator, int elemcount,
		     char *elements[]);
char *lvm_lv_add(void *stack, const char *fstype, unsigned int mbminsize,
		 unsigned int mbmaxsize);

void *lvm_pv_stack_new(void);
int lvm_pv_stack_isempty(void *stack);
int lvm_pv_stack_push(void *stack, const char *vgname, const char *devpath);
int lvm_pv_stack_pop(void *stack, char **vgname, char **devpath);
int lvm_pv_stack_delete(void *);

void *lvm_lv_stack_new(void);
int lvm_lv_stack_isempty(void *stack);
int lvm_lv_stack_push(void *stack, const char *vgname, const char *lvname,
		      const char *fstype, unsigned int mbminsize,
		      unsigned int mbmaxsize);
int lvm_lv_stack_pop(void *stack, char **vgname, char **lvname,
		     char **fstype, unsigned int *mbminsize,
		     unsigned int *mbmaxsize);
int lvm_lv_stack_delete(void *);

/* andread@linpro.no */
int lvm_get_free_space_list(char*, struct disk_info_t*);
void *reduce_disk_usage_size(struct disk_info_t*,
			     struct diskspace_req_s[],
			     double);
void *lvm_vg_stack_new(void);
int lvm_vg_stack_isempty(void *stack);
int lvm_vg_stack_push(void *stack, const char *vgname);
int lvm_vg_stack_pop(void *stack, char **vgname);
int lvm_vg_stack_delete(void *);


/* from makepath.c */
#include <sys/stat.h>
#include <sys/types.h>
int make_path(const char *pathname, mode_t mode);

/* ext3 is not supported by libparted v1.4, nor v1.6. */
#define DEFAULT_FS "ext2"

/* from evaluator.c */
double evaluate(const char *expression);

#endif /* AUTOPARTKIT_H */
