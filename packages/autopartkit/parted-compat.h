#ifndef _PARTED_COMPAT_H
#define _PARTED_COMPAT_H

/*
 * Make sure this works with both old and new API, even if the source
 * uses the new API.
 */
#if ! defined(HAVE_PED_DISK_COMMIT)
/* 1.4 have ped_disk_write(), while 1.6 have ped_disk_commit() */
/* Rewrite 1.6 API to 1.4 API */
#define ped_disk_new(dev)                     ped_disk_open(dev)
#define ped_disk_destroy(disk)                ped_disk_close(disk)
#define ped_disk_new_fresh(dev, fstype)       ped_disk_create(dev, fstype)
#define ped_file_system_resize(fs, geom, foo) \
             ped_file_system_resize(fs, geom)
#define ped_disk_commit(disk)                 ped_disk_write(disk)
#define ped_file_system_create(geom, fs_type, foo) \
             ped_file_system_create(geom, fs_type)
/* This is special, as it kept the name, but changed the parameter type */
#define PED_CONSTRAINT_ANY(disk, dev)         ped_constraint_any(disk)
/* These are removed in 1.6 */
#define PED_INIT()                            ped_init()
#define PED_DONE()                            ped_done()
#else
#define PED_CONSTRAINT_ANY(disk, dev) ped_constraint_any(dev)
#define PED_INIT()
#define PED_DONE()
/* Missing in v1.6.1 */
#  define PED_PARTITION_PRIMARY 0
#endif

#endif /* _PARTED_COMPAT_H */
