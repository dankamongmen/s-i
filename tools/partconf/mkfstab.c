
#include "mkfstab.h"

int get_filesystems(struct fstab_entry *entries[], int pos, int max_entries) {
	FILE *fmounts = NULL;
	char line[1024];
	int count_entries = pos;
	struct fstab_entry *dummy;

	fmounts = fopen("/proc/mounts", "r");
	if(fmounts == NULL)
		return(0);

	while(fgets(line, 1024, fmounts) != NULL) {
		char filesystem[1024];
		char mountpoint[1024];
		char typ[1024];
		char options[1024];

		if(count_entries >= MAX_ENTRIES)
			break;

		dummy = malloc(sizeof(*dummy));
		if(dummy == NULL)
			break;

		dummy->filesystem = NULL;
		dummy->mountpoint = NULL;
		dummy->typ = NULL;
		dummy->options = NULL;
		dummy->dump = 0;
		dummy->pass = 0;

		sscanf(line, "%s %s %s %s %*s %*s",
			filesystem, mountpoint, typ, options);

		if(strlen(filesystem) > 0)
			dummy->filesystem = strdup(filesystem);
		if(strlen(mountpoint) > 0)
			dummy->mountpoint = strdup(mountpoint);
		if(strlen(typ) > 0)
			dummy->typ = strdup(typ);

		/* handle reiserfs */
		if(strstr(dummy->typ, "reiserfs") && strstr(dummy->mountpoint, "/boot")) {
			dummy->options = strdup("notail");
		} else {
			if(strlen(options) > 0) {
				dummy->options = strdup(options);
			} else {
				dummy->options = strdup("defaults");
			}
		}

		entries[count_entries++] = dummy;
	}

	fclose(fmounts);
	return(count_entries);
}

int get_swapspaces(struct fstab_entry *entries[], int pos, int max_entries) {
	FILE *fswaps = NULL;
	char line[1024];
	int count_entries = pos;

	fswaps = fopen("/proc/swaps", "r");
	if(fswaps == NULL)
		return(0);

	while(fgets(line, 1024, fswaps) != NULL) {
		char filesystem[1024];
		struct fstab_entry *dummy;

		if(count_entries >= MAX_ENTRIES)
			break;

		dummy = malloc(sizeof(*dummy));
		if(dummy == NULL)
			break;

		dummy->filesystem = NULL;
		dummy->mountpoint = NULL;
		dummy->typ = NULL;
		dummy->options = NULL;
		dummy->dump = 0;
		dummy->pass = 0;

		sscanf(line, "%s %*s %*s %*s %*s", filesystem);

		/* skip first header line */
		if(filesystem[0] != '/')
			continue;

		if(strlen(filesystem) > 0)
			dummy->filesystem = strdup(filesystem);
		dummy->mountpoint = strdup("none");
		dummy->typ = strdup("swap");
		dummy->options = strdup("sw");

		entries[count_entries++] = dummy;
	}

	fclose(fswaps);
	return(count_entries);
}

int get_otherdevices(struct fstab_entry *entries[], int pos, int max_entries) {
	int count_entries = pos;
	struct fstab_entry *entry = NULL;

	entry = static_entries;
	while(entry->filesystem != NULL) {
		struct fstab_entry *dummy = NULL;

		if(count_entries >= MAX_ENTRIES)
			break;

		dummy = malloc(sizeof(*dummy));
		if(dummy == NULL)
			break;

		dummy->filesystem = NULL;
		dummy->mountpoint = NULL;
		dummy->typ = NULL;
		dummy->options = NULL;
		dummy->dump = 0;
		dummy->pass = 0;

		if(strlen(entry->filesystem) > 0)
			dummy->filesystem = strdup(entry->filesystem);
		if(strlen(entry->mountpoint) > 0)
			dummy->mountpoint = strdup(entry->mountpoint);
		if(strlen(entry->typ) > 0)
			dummy->typ = strdup(entry->typ);
		if(strlen(entry->options) > 0) {
			dummy->options = strdup(entry->options);
		} else {
			dummy->options = strdup("defaults");
		}

		entries[count_entries++] = dummy;
		entry++;
	}

	return(count_entries);
}

void mapdevfs(struct fstab_entry *entry) {
	char *cmd = NULL;
	FILE *pfile = NULL;
	char device[PATH_MAX];

	if(entry->filesystem == NULL)
		return;

	asprintf(&cmd, "%s %s 2>/dev/null",
		"/home/sar/installer/utils/mapdevfs", strdup(entry->filesystem));

	pfile = popen(cmd, "r");
	if(pfile == NULL)
		return;
	fscanf(pfile, "%s", device);
	pclose(pfile);

	if(strlen(device) > 0) {
		free(entry->filesystem);
		entry->filesystem = NULL;
		entry->filesystem = strdup(device);
	}
}

int main(int argc, char *argv[]) {
	struct fstab_entry *entries[MAX_ENTRIES];
	int count = 0, i = 0;
	FILE *outfile = NULL;

	count = get_filesystems(entries, count, MAX_ENTRIES);
	count = get_swapspaces(entries, count, MAX_ENTRIES);
	count = get_otherdevices(entries, count, MAX_ENTRIES);

	outfile = fopen(FSTAB_FILE, "w");
	if(outfile == NULL)
		return(0);

	fprintf(outfile, HEADER, FSTAB_FILE);
	for(i=0; i<count; i++) {
		int pass = 2;

		mapdevfs(entries[i]);
		if((strlen(entries[i]->mountpoint) == 1) && 
		(entries[i]->mountpoint[0] == '/')) {
			pass = 1;
		}
		fprintf(outfile, "%s\t%s\t%s\t%s\t%d %d\n",
			entries[i]->filesystem, entries[i]->mountpoint,
			entries[i]->typ, entries[i]->options,
			0, pass);
	}

	fclose(outfile);

	return(EXIT_SUCCESS);
}

