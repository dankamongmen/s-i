#include "libkdetect.h"

typedef struct devices_s {
	int major;
	char *major_name;
	void *driver;
	struct devices_s *next;
} devices_t;

typedef struct disk_s {
	char *device_name;
	char *model;
	unsigned long capacity;
	struct disk_s *next;
} disk_t;

typedef struct ide_s {
	disk_t *master;
	disk_t *slave;
} ide_t;

typedef struct raid_disk_s {
	char *device_name;
	int mode;
	int nr_raid_disks;
	long nr_blocks;
	disk_t **disks;
} raid_disk_t;

typedef struct raid_s {
	int supported_modes;
	raid_disk_t *disks;
} raid_t;

char *readline(int fd);
int open_proc_file(const char *filename);

int parse_proc_devices(devices_t *devices);
int parse_proc_ide(devices_t *devices);
int parse_proc_mdstat(devices_t *devices);

