
#include "main.h"

static PedExceptionOption my_exception_handler(PedException* ex) {
	if (ex->type < PED_EXCEPTION_ERROR) {
		return PED_EXCEPTION_IGNORE;
	}

	return PED_EXCEPTION_CANCEL;
}

char *get_architecture() {
	FILE *pcmd;
	char arch[256];

	pcmd = popen("/usr/bin/udpkg --print-architecture 2>/dev/null", "r");
	if(pcmd == NULL)
		return(NULL);
	if(fscanf(pcmd, "%s", arch) < 1)
		return(NULL);
	pclose(pcmd);

	return(strdup(arch));
}

int get_all_disks(PedDevice *discs[], int max_disks) {
	DIR *devdir;
	struct dirent *direntry;
	int disk_count = 0;

	devdir = opendir("/dev/discs");
	if(devdir == NULL) {
		di_log(DI_LOG_LEVEL_ERROR, "Failed to open disc directory");
		return(0);
	}

	while((direntry = readdir(devdir)) != NULL) {
		char *fullname = NULL;

		if(direntry->d_name[0] == '.')
			continue;

		asprintf(&fullname, "%s/%s/%s", "/dev/discs",
			direntry->d_name, "disc");

		if ((discs[disk_count] = ped_device_get(fullname)))
			disk_count++;
	}

	closedir(devdir);

	return(disk_count);
}

char *build_choice(PedDevice *dev) {
	char *string = NULL;
	int i;

	asprintf(&string, "%s (%s/ %.0fMB)",
		dev->path, dev->model,
		(dev->length-1)*1.0/MEGABYTE_SECTORS);

	/* remove ",", this will confuse debconf */
	for(i=0; i<strlen(string); i++) {
		if(string[i] == ',')
			string[i] = ';';
	}

	return(strdup(string));
}

char *extract_choice(const char *choice) {
	char *blank = NULL;
	char device[PATH_MAX];
	int i;

	/* FIXME: something is wrong here */
	for(i=0; i<PATH_MAX; i++)
		device[i] = '\0';

	blank = strchr(choice, 32);
	if(blank == NULL) {
		return(strdup(choice));
	}

	strncpy(device, choice, blank-choice);
	return(strdup(device));
}

char *execute_fdisk() {
	char *fdiskcmd = NULL;

	/*
	 *
	 * three-way fdisk running
	 *
	 */

	/* try to run arch fdisk command */
	asprintf(&fdiskcmd, "%s/%s.sh", FDISK_PATH, get_architecture());
	if(access(fdiskcmd, R_OK) == 0) {
		di_log(DI_LOG_LEVEL_INFO, "Using architecture depend fdisk configuration!");
		return(fdiskcmd);
	}

	/* fall back to common script, if exist */
	free(fdiskcmd);
	asprintf(&fdiskcmd, "%s/%s", FDISK_PATH, "common.sh");
	if(access(fdiskcmd, R_OK) == 0) {
		di_log(DI_LOG_LEVEL_INFO, "Using default fdisk configuration!");
		return(fdiskcmd);
	}

	/* fall back to >fdisk< */
	free(fdiskcmd);
	asprintf(&fdiskcmd, "%s", "/usr/sbin/fdisk");
	di_log(DI_LOG_LEVEL_INFO, "Fall back to default fdisk");
	return(fdiskcmd);
}

int main(int argc, char *argv[]) {
	int disks_count, i;
	PedDevice *discs[MAX_DISKS];
	char *choices = NULL;

        di_system_init(basename(argv[0]));
	/* initialize debconf */
	debconf = debconfclient_new();

	/* hide libparted errors */
	ped_exception_set_handler(my_exception_handler);

	/* collect all discs from system, and build choices list */
	disks_count = get_all_disks(discs, MAX_DISKS);
	if(disks_count < 1) {
		debconf_fset(debconf, "partitioner/nodiscs", "seen", "false");
		debconf_set(debconf, "partitioner/nodiscs", "false");
		debconf_input(debconf, "high", "partitioner/nodiscs");
		debconf_go(debconf);
		return(EXIT_SUCCESS);
	}

	for(i=0; i<disks_count; i++) {
		char *tmp = NULL;

		tmp = build_choice(discs[i]);
		if(tmp == NULL)
			continue;

		if(choices == NULL) {
			asprintf(&choices, "%s", tmp);
		} else {
			asprintf(&choices, "%s, %s", choices, tmp);
		}

		free(tmp);
	}
	asprintf(&choices, "%s, %s", choices, "Finish");

	while(1) {
		char *cmd_script = NULL;
		char *cmd = NULL;
		char *disk = NULL;

		debconf_subst(debconf, "partitioner/disc", "DISCS", choices);
		debconf_fset(debconf, "partitioner/disc", "seen", "false");
		debconf_set(debconf, "partitioner/disc", "false");
		debconf_input(debconf, "high", "partitioner/disc");
		debconf_go(debconf);
		debconf_get(debconf, "partitioner/disc");

		/* exit the endless loop */
		if(strstr(debconf->value, "Finish"))
			break;

		cmd_script = execute_fdisk();
		disk = extract_choice(debconf->value);
		asprintf(&cmd, "/bin/sh %s %s </dev/tty >/dev/tty 2>/dev/tty", cmd_script, disk);

		i = system(cmd);
		if(i != 0) {
			debconf_subst(debconf, "partitioner/fdiskerr", "DISC", disk);
			debconf_fset(debconf, "partitioner/fdiskerr", "seen", "false");
			debconf_set(debconf, "partitioner/fdiskerr", "false");
			debconf_input(debconf, "high", "partitioner/fdiskerr");
			debconf_go(debconf);
		}

		if(disk != NULL) {
			free(disk);
			disk = NULL;
		}
	}

	return(EXIT_SUCCESS);
}

