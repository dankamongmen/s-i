/* Program to search for disks with a correct yaboot setup, and then
   prompt the user for which one to install yaboot on.

   Copyright (C) 2003 Thorsten Sauter <tsauter@gmx.net>
   Copyright (C) 2002 Colin Walters <walters@gnu.org>
*/

#define _GNU_SOURCE

#include <parted/parted.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <cdebconf/debconfclient.h>
#include <debian-installer.h>

#define MAX_DISCS 64
#define TARGET "/target"

extern int errno;
static struct debconfclient *debconf;
PedPartition *bparts[MAX_DISCS];
int part_count = 0;

int read_physical_discs();
int read_all_partitions(const char *devname);
int is_bootable(const PedPartition *part);
char *find_rootpartition();
int generate_yabootconf(const char *boot_devfs, const char *root_devfs);
int update_kernelconf();
char *build_choice(PedPartition *part);
char *extract_choice(const char *choice);
int main(int argc, char **argv);

