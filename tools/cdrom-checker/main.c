/* cdrom-checker - verify debian cdrom's
 * Copyright (C) 2003 Thorsten Sauter <tsauter@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include "main.h"


void detect_cdrom() {
	FILE *mount;
	char line[1024];

	mount = fopen("/proc/mounts", "r");
	while(fgets(line, sizeof(line), mount) != 0) {
		char device[1024], mpoint[1024];
		sscanf(line, "%s %s %*s", device, mpoint);
		if(strstr(mpoint, TARGET)) {
			asprintf(&cdrom_device, "%s", device);
			break;
		}
	}
	fclose(mount);
	
	if(cdrom_device == NULL) {
		di_log("Unable to detect cdrom device!");
		exit(EXIT_SUCCESS);
	}
}

int mount_cdrom() {
	int status = 0;
	char *cmd = NULL;

	asprintf(&cmd, "/bin/grep -q '^%s' /proc/mounts",
		cdrom_device);
	status = system(cmd);
	if(status == 0) {
		free(cmd);
		return(EXIT_SUCCESS);
	}

	/* free up cmd allocated memory */
	if(cmd != NULL) {
		free(cmd);
		cmd = NULL;
	}

	/* ask if we should mount the device */
	debconf->command(debconf, "fset", "cdrom-checker/askmount",
		"seen", "false", NULL);
	debconf->command(debconf, "set", "cdrom-checker/askmount",
		"false", NULL);
	debconf->command(debconf, "input", "high", "cdrom-checker/askmount", NULL);
	debconf->command(debconf, "go", NULL);
	debconf->command(debconf, "get", "cdrom-checker/askmount", NULL);
	
	asprintf(&cmd, "mount -t iso9660 %s %s -o ro 1>/dev/null 2>&1",
		cdrom_device, TARGET);
	status = system(cmd);
	if(status == 0)
		return(EXIT_SUCCESS);

	/* unable to mount the cdrom device */
	debconf->command(debconf, "fset", "cdrom-checker/mntfailed",
		"seen", "false", NULL);
	debconf->command(debconf, "SUBST", "cdrom-checker/mntfailed",
		"CDROM", cdrom_device, NULL);
	debconf->command(debconf, "input", "high", "cdrom-checker/mntfailed", NULL);
	debconf->command(debconf, "go", NULL);
	return(EXIT_FAILURE);
}

int md5file_getlines(FILE *md5file) {
	char line[1024];
	int lines = 0;

	rewind(md5file);
	while(fgets(line, sizeof(line), md5file) != 0)
		lines++;

	return(lines);
}

int valid_cdrom() {
	struct stat dir;

	/* test ".disk" directory */
	if(stat(".disk", &dir) != 0)
		return(EXIT_FAILURE);
	if(!S_ISDIR(dir.st_mode))
		return(EXIT_FAILURE);

	/* test ".disk" directory */
	if(stat("debian", &dir) != 0)
		return(EXIT_FAILURE);
	if(!S_ISDIR(dir.st_mode))
		return(EXIT_FAILURE);

	/* test "md5sum.txt" directory */
	if(stat("md5sum.txt", &dir) != 0)
		return(EXIT_FAILURE);
	if(!S_ISREG(dir.st_mode))
		return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}

int check_cdrom() {
	FILE *md5file = NULL;
	int lines, status = 0;
	char line[1024];
	char filename[1024];

	md5file = fopen("md5sum.txt", "r");
	if(md5file == NULL) {
		debconf->command(debconf, "input", "high",
			"cdrom-checker/md5file_failed", NULL);
		debconf->command(debconf, "go", NULL);
		exit(EXIT_FAILURE);
	}

	lines = md5file_getlines(md5file);
	debconf->commandf(debconf,
		"PROGRESS START 0 %d cdrom-checker/progress_title", lines);

	rewind(md5file);
	while(fgets(line, sizeof(line), md5file) != 0) {
		char *cmd;

		status = 0;
		sscanf(line, "%*s %s", filename);
		debconf->command(debconf, "SUBST", "cdrom-checker/progress_step",
				"FILE", filename, NULL);
		debconf->command(debconf, "PROGRESS", "INFO",
				"cdrom-checker/progress_step", NULL);
		asprintf(&cmd, "echo '%s' | md5sum -c 1>/dev/null 2>&1", line);
		if(system(cmd) != 0) {
			status = 1;
			break;
		}
		debconf->command(debconf, "PROGRESS", "STEP", "1", NULL);
	}

	fclose(md5file);
	debconf->command(debconf, "PROGRESS STOP", NULL);

	/* print the result of the command */
	if(status != 0) {
		debconf->command(debconf, "fset", "cdrom-checker/missmatch",
			"seen", "false", NULL);
		debconf->command(debconf, "SUBST", "cdrom-checker/missmatch",
			"FILE", filename, NULL);
		debconf->command(debconf, "input", "high",
			"cdrom-checker/missmatch", NULL);
	} else {
		debconf->command(debconf, "fset", "cdrom-checker/passed",
			"seen", "false", NULL);
		debconf->command(debconf, "input", "high",
			"cdrom-checker/passed", NULL);
	}
	debconf->command(debconf, "go", NULL);

	return(status);
}

int main(int argc, char **argv) {
	/* initialize the debconf frontend */
	debconf = debconfclient_new();
	debconf->command(debconf, "title", "CD-ROM Checker", NULL);

	/* ask if we should proceed the checking */
	debconf->command(debconf, "fset", "cdrom-checker/start",
		"seen", "false", NULL);
	debconf->command(debconf, "set", "cdrom-checker/start", "false", NULL);
	debconf->command(debconf, "input", "high", "cdrom-checker/start", NULL);
	debconf->command(debconf, "go", NULL);
	debconf->command(debconf, "get", "cdrom-checker/start", NULL);
	if(!strstr(debconf->value, "true")) {	/* return to main-menu */
		return(EXIT_SUCCESS);
	}

	detect_cdrom();

	/* goeing into main-loop */
	while(1) {
		/* try to mount the cdrom and exit on error */
		if(mount_cdrom() != EXIT_SUCCESS)
			break;

		/* go to cdrom mount-point */
		chdir(TARGET);

		/* is this a valid Debian cdrom? */
		if(valid_cdrom() != EXIT_SUCCESS) {
			debconf->command(debconf, "fset", "cdrom-checker/wrongcd", "seen",
				"false", NULL);
			debconf->command(debconf, "set", "cdrom-checker/wrongcd", "false", NULL);
			debconf->command(debconf, "input", "high", "cdrom-checker/wrongcd", NULL);
			debconf->command(debconf, "go", NULL);
			break;
		}

		check_cdrom();

		/* make sure cdrom isn't bussy anymore, then umount it */
		chdir("/");
		di_execlog("umount /cdrom");

		debconf->command(debconf, "fset", "cdrom-checker/nextcd",
			"seen", "false", NULL);
		debconf->command(debconf, "set", "cdrom-checker/nextcd",
			"false", NULL);
		debconf->command(debconf, "input", "high",
			"cdrom-checker/nextcd", NULL);
		debconf->command(debconf, "go", NULL);
		debconf->command(debconf, "get", "cdrom-checker/nextcd", NULL);
		if(!strstr(debconf->value, "true"))
			break;
	}

	debconf->command(debconf, "fset", "cdrom-checker/firstcd",
		"seen", "false", NULL);
	debconf->command(debconf, "set", "cdrom-checker/firstcd", "false", NULL);
	debconf->command(debconf, "input", "high", "cdrom-checker/firstcd", NULL);
	debconf->command(debconf, "go", NULL);
	di_execlog("udpkg --force-configure --configure cdrom-detect");

	return(EXIT_SUCCESS);
}

