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

int get_all_hintings(struct partition *parts[], 
	const int curr, const int max_parts);
int write_hintings_file(struct partition *parts[], const int count);
int add_hinting_partition(struct partition *parts[], const int curr);
int edit_hinting_partition(struct partition *parts[], const int curr, const int id);

