
#include "hinting.h"

extern struct debconfclient *debconf;

int
get_all_hintings(struct partition *parts[], const int curr, const int max_parts)
{
	int count = curr;
	FILE *hintfile = NULL;
	char line[1024];

	//return(curr);

	hintfile = fopen("/var/lib/partconf/fstab.d/partconf", "r");
	if(hintfile == NULL) {
		if(errno != ENOENT)
			fprintf(stderr, "Error while opening file: %m");
		return(count);
	}

	while(fgets(line, 1024, hintfile) != NULL) {
		char filesystem[1024];
		char mntpoint[1024];
		char typ[1024];
		char options[1024];
		struct partition *p;

		if(count >= max_parts)
            break;

		p = malloc(sizeof(*p));
		if(p == NULL)
			break;

		sscanf(line, "%s %s %s %s %*s", filesystem,
			mntpoint, typ, options);

		if((strlen(filesystem) == 0) || (strlen(mntpoint) == 0))
			continue;
		
		p->path = strdup(filesystem);
		if(strlen(typ) != 0)
			p->fstype = strdup(typ);
		else
			p->fstype = strdup("auto");
		p->fsid = NULL;
		p->size = 0L;
		p->hinting = 1;
		p->op.filesystem = NULL;
		p->op.mountpoint = strdup(mntpoint);
		if(strlen(options) != 0)
			p->op.mountopts = strdup(options);
		else
			p->op.mountopts = strdup("defaults");
		p->op.done = 1;
		parts[count++] = p;
	}

	fclose(hintfile);
	return(count);
}

int write_hintings_file(struct partition *parts[], const int count) {
	int i = 0;
	FILE *hintfile = NULL;

	if(count <= 0)
		return(0);

	hintfile = fopen("/var/lib/partconf/fstab.d/partconf", "w");
	if(hintfile == NULL) {
		fprintf(stderr, "Failed to write hinting file!\n");
		return(1);
	}

	for(i=0; i<count; i++) {
		struct partition *p;

		p = parts[i];
		if(p->hinting != 1)
			continue;

		fprintf(hintfile, "%s\t%s\t%s\t%s\n",
			p->path,
			p->op.mountpoint != NULL ? p->op.mountpoint : "none",
			p->fstype != NULL ? p->fstype : "auto",
			p->op.mountopts != NULL ? p->op.mountopts : "defaults");

		makedirs(p->op.mountpoint);
	}

	fclose(hintfile);
	return(0);
}

int add_hinting_partition(struct partition *parts[], const int curr) {
	int count = curr;
	int i = 0;
	struct partition *p = NULL;

	p = malloc(sizeof(*p));
	if(p == NULL)
		return(count);
	p->fsid = NULL;
	p->size = 0L;
	p->hinting = 1;
	p->op.filesystem = NULL;
	p->op.done = 1;

	//
	// query the values from the user
	//
	debconf->command(debconf, "fset", "partconf/hint-device", "seen",
		"false", NULL);
	debconf->command(debconf, "set", "partconf/hint-device", "", NULL);
	debconf->command(debconf, "input", "high", "partconf/hint-device", NULL);
	debconf->command(debconf, "go", NULL);
	debconf->command(debconf, "get", "partconf/hint-device", NULL);
	if(strlen(debconf->value) < 2 || debconf->value[0] != '/') {
		debconf->command(debconf, "subst", "partconf/hint-dev-invalid",
			"DEVICE", debconf->value, NULL);
		debconf->command(debconf, "input", "high",
			"partconf/hint-dev-invalid", NULL);
		debconf->command(debconf, "go", NULL);
		return(count);
	}
	p->path = strdup(debconf->value);

	debconf->command(debconf, "fset", "partconf/hint-mountpoint", "seen",
		"false", NULL);
	debconf->command(debconf, "set", "partconf/hint-mountpoint", "", NULL);
	debconf->command(debconf, "subst", "partconf/hint-mountpoint",
		"PARTITION", p->path, NULL);
	debconf->command(debconf, "input", "high", "partconf/hint-mountpoint", NULL);
	debconf->command(debconf, "go", NULL);
	debconf->command(debconf, "get", "partconf/hint-mountpoint", NULL);
	if(strlen(debconf->value) < 2 || debconf->value[0] != '/') {
		debconf->command(debconf, "subst", "partconf/hint-mnt-invalid",
			"MOUNTPOINT", debconf->value, NULL);
		debconf->command(debconf, "input", "high",
			"partconf/hint-mnt-invalid", NULL);
		debconf->command(debconf, "go", NULL);
		return(count);
	}
	p->op.mountpoint = strdup(debconf->value);

	debconf->command(debconf, "fset", "partconf/hint-fstype", "seen",
		"false", NULL);
	debconf->command(debconf, "set", "partconf/hint-fstype", "auto", NULL);
	debconf->command(debconf, "subst", "partconf/hint-fstype",
		"PARTITION", p->path, NULL);
	debconf->command(debconf, "input", "high", "partconf/hint-fstype", NULL);
	debconf->command(debconf, "go", NULL);
	debconf->command(debconf, "get", "partconf/hint-fstype", NULL);
	if(strlen(debconf->value) >= 1)
		p->fstype = strdup(debconf->value);
	else
		p->fstype = strdup("auto");

	debconf->command(debconf, "fset", "partconf/hint-mntopts", "seen",
		"false", NULL);
	debconf->command(debconf, "set", "partconf/hint-mntopts", "defaults", NULL);
	debconf->command(debconf, "subst", "partconf/hint-mntopts",
		"PARTITION", p->path, NULL);
	debconf->command(debconf, "input", "high", "partconf/hint-mntopts", NULL);
	debconf->command(debconf, "go", NULL);
	debconf->command(debconf, "get", "partconf/hint-mntopts", NULL);
	if(strlen(debconf->value) >= 1)
		p->op.mountopts = strdup(debconf->value);
	else
		p->op.mountopts = strdup("defaults");

	//
	// check for doubble entries
	//
	for(i=0; i<count; i++) {
		if(parts[i]->path != NULL && strcmp(parts[i]->path, p->path) == 0) {
			debconf->command(debconf, "subst", "partconf/hint-dev-inuse",
				"DEVICE", p->path, NULL);
			debconf->command(debconf, "input", "high",
				"partconf/hint-dev-inuse", NULL);
			debconf->command(debconf, "go", NULL);
			return(count);
		}
		if(parts[i]->op.mountpoint != NULL &&
		strcmp(parts[i]->op.mountpoint, p->op.mountpoint) == 0) {
			debconf->command(debconf, "subst", "partconf/hint-mnt-inuse",
				"MOUNTPOINT", p->op.mountpoint, NULL);
			debconf->command(debconf, "input", "high",
				"partconf/hint-mnt-inuse", NULL);
			debconf->command(debconf, "go", NULL);
			return(count);
		}
	}

	/* add and return */
	parts[count++] = p;
	return(count);
}

int edit_hinting_partition(struct partition *parts[], const int curr, const int id) {
	int i = 0, count = curr;
	struct partition *p = parts[id];
	char *path = NULL;
	char *mountpoint = NULL;
	char *fstype = NULL;
	char *mountopts = NULL;

	debconf->command(debconf, "fset", "partconf/hint-device", "seen",
		"false", NULL);
	debconf->command(debconf, "set", "partconf/hint-device",
		p->path, NULL);
	debconf->command(debconf, "input", "high", "partconf/hint-device", NULL);
	debconf->command(debconf, "go", NULL);
	debconf->command(debconf, "get", "partconf/hint-device", NULL);
	if(strlen(debconf->value) < 2 || debconf->value[0] != '/') {
		debconf->command(debconf, "subst", "partconf/hint-dev-invalid",
			"DEVICE", debconf->value, NULL);
		debconf->command(debconf, "input", "high",
			"partconf/hint-dev-invalid", NULL);
		debconf->command(debconf, "go", NULL);
		return(1);
	}
	path = strdup(debconf->value);

	debconf->command(debconf, "fset", "partconf/hint-mountpoint", "seen",
		"false", NULL);
	debconf->command(debconf, "set", "partconf/hint-mountpoint",
		p->op.mountpoint, NULL);
	debconf->command(debconf, "subst", "partconf/hint-mountpoint",
		"PARTITION", p->path, NULL);
	debconf->command(debconf, "input", "high", "partconf/hint-mountpoint", NULL);
	debconf->command(debconf, "go", NULL);
	debconf->command(debconf, "get", "partconf/hint-mountpoint", NULL);
	if(strlen(debconf->value) < 2 || debconf->value[0] != '/') {
		debconf->command(debconf, "subst", "partconf/hint-mnt-invalid",
			"MOUNTPOINT", debconf->value, NULL);
		debconf->command(debconf, "input", "high",
			"partconf/hint-mnt-invalid", NULL);
		debconf->command(debconf, "go", NULL);
		return(1);
	}
	mountpoint = strdup(debconf->value);

	debconf->command(debconf, "fset", "partconf/hint-fstype", "seen",
		"false", NULL);
	debconf->command(debconf, "set", "partconf/hint-fstype", p->fstype, NULL);
	debconf->command(debconf, "subst", "partconf/hint-fstype",
		"PARTITION", p->path, NULL);
	debconf->command(debconf, "input", "high", "partconf/hint-fstype", NULL);
	debconf->command(debconf, "go", NULL);
	debconf->command(debconf, "get", "partconf/hint-fstype", NULL);
	if(strlen(debconf->value) >= 1)
		fstype = strdup(debconf->value);
	else
		fstype = strdup("auto");

	debconf->command(debconf, "fset", "partconf/hint-mntopts", "seen",
		"false", NULL);
	debconf->command(debconf, "set", "partconf/hint-mntopts",
		p->op.mountopts, NULL);
	debconf->command(debconf, "subst", "partconf/hint-mntopts",
		"PARTITION", p->path, NULL);
	debconf->command(debconf, "input", "high", "partconf/hint-mntopts", NULL);
	debconf->command(debconf, "go", NULL);
	debconf->command(debconf, "get", "partconf/hint-mntopts", NULL);
	if(strlen(debconf->value) >= 1)
		mountopts = strdup(debconf->value);
	else
		mountopts = strdup("defaults");

	for(i=0; i<count; i++) {
		if(i == id)
			continue;

		if(parts[i]->path != NULL && strcmp(parts[i]->path, path) == 0) {
			debconf->command(debconf, "subst", "partconf/hint-dev-inuse",
				"DEVICE", path, NULL);
			debconf->command(debconf, "input", "high",
				"partconf/hint-dev-inuse", NULL);
			debconf->command(debconf, "go", NULL);
			return(1);
		}
		if(parts[i]->op.mountpoint != NULL &&
		strcmp(parts[i]->op.mountpoint, mountpoint) == 0) {
			debconf->command(debconf, "subst", "partconf/hint-mnt-inuse",
				"MOUNTPOINT", mountpoint, NULL);
			debconf->command(debconf, "input", "high",
				"partconf/hint-mnt-inuse", NULL);
			debconf->command(debconf, "go", NULL);
			return(1);
		}
	}

	free(p->path); p->path = strdup(path);
	free(p->op.mountpoint); p->op.mountpoint = strdup(mountpoint);
	free(p->fstype); p->fstype = strdup(fstype);
	free(p->op.mountopts); p->op.mountopts = strdup(mountopts);
	return(0);
}

