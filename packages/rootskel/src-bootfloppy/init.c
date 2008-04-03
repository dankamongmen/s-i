/* 
 * This is a special init for the boot floppy image.
 * It loads the main d-i initrd from either a second floppy, or a USB
 * storage device.
 *
 * Note that this code cannot use system() -- there is no shell.
 *
 * INSTALL_MEDIA_DEV can be passed as a boot parameter to force only 
 * one device or set of devices to be scanned.
 *
 * Copyright 2008 Joey Hess <joeyh@debian.org>, GPL 2+
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <unistd.h>

int debug=0;

void checkbootdebug (void) {
	char *value=getenv("BOOT_DEBUG");
	if (value != NULL) {
		debug=atoi(value);
	}
}

void error (const char *reason) {
	if (reason)
		fprintf(stderr, "%s\n", reason);
	fprintf(stderr,"Giving up!\n");
	exit(1);
}

int waitchild (void) {
	int status;
	wait(&status);
	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}
	else {
		return 255;
	}
}

/* Not a real modprobe, you have to tell it the path to the module
 * within the modules directory. */
int modprobe (char *module) {
	char *m;
	struct utsname buf;

	uname(&buf);
	asprintf(&m, "/lib/modules/%s/%s.ko", buf.release, module);

	if (fork() == 0) {
		execlp("insmod", "insmod", m, NULL);
		exit(1);
	}
	else {
		return waitchild();
	}
}

int mount (char *type, char *device, char *dir, char *options) {
	if (fork() == 0) {
		if (options)
			execlp("mount", "mount", "-t", type, "-o", options, device, dir, NULL);
		else
			execlp("mount", "mount", "-t", type, device, dir, NULL);
		exit(1);
	}
	else {
		return waitchild();
	}
}

int umount (char *dir) {
	if (fork() == 0) {
		execlp("umount", "umount", dir, NULL);
		exit(1);
	}
	else {
		return waitchild();
	}
}

char *readsysfile (char *file) {
	static char ret[256];
	int fd;
	if (((fd=open(file, O_RDONLY)) == -1) ||
	    (read(fd, ret, 255) == -1) ||
	    (close(fd) == -1))
		return NULL;
	else
		return ret;
}

int writesysfile (char *file, char *value) {
	int fd;
	if (((fd=open(file, O_WRONLY)) == -1) ||
	    (write(fd, value, strlen(value)) == -1) ||
	    (close(fd) == -1))
		return 0;
	else
		return 1;
}

void setup (void) {
	int i;
	
	/* Connect stdio to console. */
	close(0);
	close(1);
	close(2);
	open("/dev/console", O_RDONLY);
	open("/dev/console", O_WRONLY);
	open("/dev/console", O_WRONLY);
	
	checkbootdebug();
	/* set path, including path to klibc programs */
	setenv("PATH", "/usr/bin:/bin:/usr/sbin:/sbin:/usr/lib/klibc/bin", 1);

	if (mount("proc", "proc", "proc", NULL) != 0 ||
	    mount("sysfs", "sysfs", "sys", NULL) != 0) {
		error(NULL);
	}
	if (modprobe("kernel/drivers/block/floppy") != 0) {
		error(NULL);
	}

	for (i=0; i < 8; i++) {
		char *sysfile, *dev;
		asprintf(&sysfile, "/sys/block/fd%i/dev", i);
		dev=readsysfile(sysfile);
		if (dev != NULL) {
			char *major=strtok(dev, ":\n");
			char *minor=strtok(NULL, ":\n");
			asprintf(&dev, "/dev/fd%i", i);
			/* makedev is provided by klibc sysmacros.h */
			if (mknod(dev, S_IFBLK | 0666,
			          makedev(atoi(major), atoi(minor))) != 0)
				error("mknod");
		}
	}
}

void startinstaller (void) {
	umount("proc");
	umount("sys");
	printf("Starting the installer...\n");
	if (execlp("run-init", "run-init", "mnt", "/init", NULL) != 0) {
		error("run-init failed");
	}
}

void promptfloppy (void) {
	char buf[2];
	static char *printk="/proc/sys/kernel/printk";
	char *oldprintk;
	fd_set set;
	struct timeval to;
		
	oldprintk=readsysfile(printk);
	writesysfile(printk, "0\n");

	printf("\nInsert the Root floppy\n");
	printf("Press Enter when ready or wait 30 seconds: ");
	to.tv_sec = 30;
	to.tv_usec = 0;
	FD_ZERO(&set);
	FD_SET(0, &set);
	if (select(1, &set, NULL, NULL, &to) == 1) {
		read(0, buf, 1);
	}
	printf("\n");

	if (oldprintk) {
		writesysfile(printk, oldprintk);
	}
}

int loadfloppy_dev (char *dev) {
	int i, status, err=0;
	int cat_pid, zcat_pid, cpio_pid;
	int pipe_a[2], pipe_b[2];
	struct stat buf;

	if (stat(dev, &buf) != 0 ||
	    (! S_ISBLK(buf.st_mode) && ! S_ISCHR(buf.st_mode)))
		return 0;
	printf("Trying %s...", dev);

	if (mount("tmpfs", "tmpfs", "mnt", "size=100M") != 0 &&
	    mount("shm", "shm", "mnt", NULL) != 0) {
		error("failed to mount ramdisk");
	}

	/* Fork off cat because klibc zcat refuses to
	 * read devices directly, and because we want to
	 * see stderr from cat when reading the floppy. */
	if (pipe(pipe_a) != 0)
		error("pipe");
	if ((cat_pid = fork()) == 0) {
		/* cat subprocess */
		close(pipe_a[0]); /* read end */
		/* connect pipe to stdout */
		close(1);
		dup2(pipe_a[1], 1); 
		execlp("cat", "cat", dev, NULL);
		exit(1);
	}
	else {
		close(pipe_a[1]); /* write end */
	}
	
	/* Next, use zcat to decompress. */
	if (pipe(pipe_b) != 0)
		error("pipe");
	if ((zcat_pid = fork()) == 0) {
		/* zcat subprocess */
		close(2); /* ignore null padding complaints on stderr */
		close(pipe_b[0]); /* read end */
		/* connect pipes to stdio */
		close(0);
		dup2(pipe_a[0], 0);
		close(1);
		dup2(pipe_b[1], 1);
		close(pipe_a[0]);
		close(pipe_b[1]);
		execlp("zcat", "zcat", NULL);
		exit(1);
	}
	else {
		close(pipe_a[0]); /* read end */
		close(pipe_b[1]); /* write end */
	}
	
	/* Last in the pipeline is cpio. With -V it will output
	 * dots as the floppy is read. */
	if ((cpio_pid = fork()) == 0) {
		/* cpio subprocess */
		if (chdir("mnt") != 0)
			perror("chdir");
		/* connect pipe to stdin */
		close(0);
		dup2(pipe_b[0], 0);
		execlp("cpio", "cpio", "-i", "-V", NULL);
		exit(1);
	}
	else {
		close(pipe_b[0]); /* read end */
	}

	/* Wait for child processes to exit, and check the cat and cpio for
	 * errors. */
	waitpid(cat_pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		err=1;
	waitpid(zcat_pid, &status, 0);
	waitpid(cpio_pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		err=1;

	if (err) {
		fprintf(stderr, "Error extracting image (wrong floppy?)\n");
		umount("mnt");
		return 0;
	}
	else {
		return 1;
	}
}
	
int loadfloppy (void) {
	int i, ret, tried=0;
	char *dev=getenv("INSTALL_MEDIA_DEV");
	if (dev != NULL) {
		ret=loadfloppy_dev(dev);
		tried++;
		if (ret)
			return ret;
	}
	else {
		for (i=0; i < 8; i++) {
			asprintf(&dev, "/dev/fd%i", i);
			ret=loadfloppy_dev(dev);
			tried++;
			free(dev);
			if (ret)
				return ret;
		}
	}

	if (tried == 0)
		fprintf(stderr, "No devices found!\n");

	return 0;
}
	
int main (int argv, char **argc) {
	int loaded=0;

	setup();

	printf("\n\n\n\n\n");
	while (! loaded) {
		promptfloppy();
		loaded=loadfloppy();
	}

	startinstaller();
	exit(1); /* not reached */
}
