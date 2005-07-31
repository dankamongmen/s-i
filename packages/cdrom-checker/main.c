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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
		di_log(DI_LOG_LEVEL_WARNING, "Unable to detect cdrom device!");
		exit(EXIT_SUCCESS);
	}
}

int mount_cdrom() {
	int status = 0;
	char *cmd = NULL;

	asprintf(&cmd, "grep -q '^%s' /proc/mounts", cdrom_device);
	status = di_exec_shell(cmd);
        di_free(cmd);
	if(WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return(EXIT_SUCCESS);

	/* ask if we should mount the device */
	debconf_set(debconf, "cdrom-checker/askmount", "false");
	debconf_input(debconf, "critical", "cdrom-checker/askmount");
	debconf_go(debconf);
	debconf_get(debconf, "cdrom-checker/askmount");
	
	asprintf(&cmd, "mount -t iso9660 %s %s -o ro 1>/dev/null 2>&1",
		cdrom_device, TARGET);
	status = di_exec_shell(cmd);
        di_free(cmd);
	if(WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return(EXIT_SUCCESS);

	/* unable to mount the cdrom device */
	debconf_subst(debconf,"cdrom-checker/mntfailed", "CDROM", cdrom_device);
	debconf_input(debconf, "high", "cdrom-checker/mntfailed");
	debconf_go(debconf);
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

	/* test "debian" directory */
	if(stat("debian", &dir) != 0)
		return(EXIT_FAILURE);
	if(!S_ISDIR(dir.st_mode))
		return(EXIT_FAILURE);

	/* test "md5sum.txt" file */
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
		debconf_input(debconf, "high", "cdrom-checker/md5file_failed");
		debconf_go(debconf);
		exit(EXIT_FAILURE);
	}

	lines = md5file_getlines(md5file);
	debconf_progress_start(debconf, 0, lines,  "cdrom-checker/progress_title");

	rewind(md5file);
	while(fgets(line, sizeof(line), md5file) != 0) {
		char *cmd;

		status = 0;
		sscanf(line, "%*s %s", filename);
		debconf_subst(debconf, "cdrom-checker/progress_step", "FILE", filename);
		debconf_progress_info(debconf, "cdrom-checker/progress_step");
		asprintf(&cmd, "echo '%s' | md5sum -c 1>/dev/null 2>&1", line);
		if(system(cmd) != 0) {
			status = 1;
			break;
		}
		debconf_progress_step(debconf, 1);
	}

	fclose(md5file);
	debconf_progress_stop(debconf);

	/* print the result of the command */
	if(status != 0) {
		debconf_subst(debconf, "cdrom-checker/mismatch", "FILE", filename);
		debconf_input(debconf, "high", "cdrom-checker/mismatch");
	} else {
		debconf_input(debconf, "critical", "cdrom-checker/passed");
	}
	debconf_go(debconf);

	return(status);
}

int main(int argc, char **argv) {
	system("touch /tmp/test");
	system("logger init");
        di_system_init(basename(argv[0]));
	/* initialize the debconf frontend */
	debconf = debconfclient_new();
	system("logger after");

	/* ask if we should proceed with the checking */
	debconf_set(debconf, "cdrom-checker/start", "false");
	debconf_input(debconf, "critical", "cdrom-checker/start");
	debconf_go(debconf);
	debconf_get(debconf, "cdrom-checker/start");
	if(!strstr(debconf->value, "true")) {	/* return to main-menu */
		return(EXIT_SUCCESS);
	}
	system("logger after-quest");

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
			debconf_set(debconf, "cdrom-checker/wrongcd", "false");
			debconf_input(debconf, "high", "cdrom-checker/wrongcd");
			debconf_go(debconf);
			break;
		}

		check_cdrom();

		/* make sure cdrom isn't busy anymore, then umount it */
		chdir("/");
		di_exec_shell_log("umount /cdrom");

		debconf_set(debconf, "cdrom-checker/nextcd", "false");
		debconf_input(debconf, "high", "cdrom-checker/nextcd");
		debconf_go(debconf);
		debconf_get(debconf,"cdrom-checker/nextcd");
		if(!strstr(debconf->value, "true"))
			break;
	}

	debconf_set(debconf, "cdrom-checker/firstcd", "false");
	debconf_input(debconf, "high", "cdrom-checker/firstcd");
	debconf_go(debconf);
	di_exec_shell_log("udpkg --force-configure --configure cdrom-detect");

	return(EXIT_SUCCESS);
}

