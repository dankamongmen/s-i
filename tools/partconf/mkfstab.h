
#ifndef __MKFSTAB_H__
#define __MKFSTAB_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <string.h>

#define TARGET "/target"
#define MAX_ENTRIES 256
#define FSTAB_FILE "/etc/fstab"
#define HEADER \
	"# %s: static file system information.\n" \
	"#\n" \
	"# <file system> <mount point>   <type>  <options>       <dump>  <pass>\n"

struct fstab_entry {
	char *filesystem;
	char *mountpoint;
	char *typ;
	char *options;
	int dump;
	int pass;
};

/* define all static/always used devices here */
struct fstab_entry static_entries[] = {
	{ "/dev/cdroms/cdrom0", "/dev/pts", "devpts", "defaults", 0, 0 },
	{ "/dev/floopy/0", "/floppy", "auto", "rw,user,noauto", 0, 0 },
	{ "/dev/cdroms/cdrom0", "/cdrom", "auto", "ro,user,noauto", 0, 0 },
	{ NULL, NULL, NULL, NULL, 0, 0 }
};

int get_filesystems(struct fstab_entry *entries[], int pos, int max_entries);
int get_swapspaces(struct fstab_entry *entries[], int pos, int max_entries);
int get_otherdevices(struct fstab_entry *entries[], int pos, int max_entries);
int main(int argc, char *argv[]);

#endif /* __MKFSTAB_H__ */

