/* $Id: udpkg.c,v 1.1 2000/08/24 06:23:56 tausq Exp $ */
#include "udpkg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* 
 * Main udpkg implementation routines
 */

#ifdef DODEBUG
static int do_system(const char *cmd)
{
	DPRINTF("cmd is %s\n", cmd);
	return system(cmd);
}
#endif

#if 0
static int is_file(const char *fn)
{
	struct stat statbuf;

	if (stat(fn, &statbuf) < 0) return 0;
	return S_ISREG(statbuf.st_mode);
}

static int dpkg_doinstall(struct package_t *pkg)
{
	char buf[1024];
	int r = 0;
	char *cwd = 0;

	cwd = getcwd(0, 0);
	/* chdir("/"); */
	chdir("tmp"); /* testing! */
	snprintf(buf, sizeof(buf), "ar p %s data.tar.gz|zcat|tar -xf -", pkg->file);
	if ((r = SYSTEM(buf)) != 0) goto cleanup;
	
	chdir(cwd);
	if (is_file(DPKGCIDIR "postinst"))
	{
		snprintf(buf, sizeof(buf), DPKGCIDIR "postinst configure %s", pkg->version);
	}

cleanup:
	chdir(cwd);
	if (cwd != 0) free(cwd);
	return r;
}
#endif

static int copyfile(const char *src, const char *dest)
{
	return 0;
}

static int dpkg_doconfigure(struct package_t *pkg)
{
	DPRINTF("Configuring %s\n", pkg->package);
	return 0;
}

static int dpkg_dounpack(struct package_t *pkg)
{
	int r = 1;
	char *cwd;
	FILE *infp, *outfp;
	char buf[1024];
	struct stat statbuf;

	DPRINTF("Unpacking %s\n", pkg->package);
	return 0;

	cwd = getcwd(0, 0);
	/* chdir("/"); */
	chdir("tmp/"); /* testing */
	snprintf(buf, sizeof(buf), "ar p %s data.tar.gz|zcat|tar -xf -", pkg->file);
	if (SYSTEM(buf) == 0)
	{
		pkg->status &= STATUS_STATUSMASK;
		pkg->status |= STATUS_STATUSUNPACKED;

		/* Installs the package scripts into the info directory */

	}
	chdir(cwd);
	return r;
}

static int dpkg_doinstall(struct package_t *pkg)
{
	DPRINTF("Installing %s\n", pkg->package);
	return (dpkg_dounpack(pkg) || dpkg_doconfigure(pkg));
}

static int dpkg_unpackcontrol(struct package_t *pkg)
{
	int r = 1;
	char *cwd = 0;
	char *p;
	int fd;
	char buf[1024];
	struct stat statbuf;

	p = strrchr(pkg->file, '/');
	if (p) p++; else p = pkg->file;
	p = pkg->package = strdup(p);
	while (*p != 0 && *p != '_' && *p != '.') p++;
	*p = 0;

	cwd = getcwd(0, 0);
	snprintf(buf, sizeof(buf), DPKGCIDIR "%s", pkg->package);
	DPRINTF("dir = %s\n", buf);
	if (mkdir(buf, S_IRWXU) == 0 && chdir(buf) == 0)
	{
		snprintf(buf, sizeof(buf), "ar p %s control.tar.gz|zcat|tar -xf -",
			pkg->file);
		if (SYSTEM(buf) == 0)
		{
			snprintf(buf, sizeof(buf), DPKGCIDIR "%s/control", 
				pkg->package);
			if (stat(buf, &statbuf) == 0)
			{
				pkg->control = (char *)malloc(statbuf.st_size+1);
				pkg->control[statbuf.st_size] = 0;
				if ((fd = open(buf, O_RDONLY)) > 0)
				{
					read(fd, pkg->control, statbuf.st_size);
					close(fd);

					if ((p = strstr(pkg->control, "\nVersion: ")) != 0)
						pkg->version = p+10;
					if ((p = strstr(pkg->control, "\nDepends: ")) != 0)
						pkg->depends = p+10;
					if ((p = strstr(pkg->control, "\nProvides: ")) != 0)
						pkg->provides = p+11;

					r = 0;
				}
			}
		}
	}

	chdir(cwd);
	if (cwd != 0) free(cwd);
	return r;
}


static int dpkg_install(struct package_t *pkgs)
{
	struct package_t *p, *ordered = 0;
	void *status = status_read();
	if (status == 0) return -1;
	if (SYSTEM("rm -rf -- " DPKGCIDIR) != 0 ||
	    mkdir(DPKGCIDIR, S_IRWXU) != 0)
	{
		perror("mkdir");
		return 1;
	}
	
	/* Stage 1: parse all the control information */
	for (p = pkgs; p != 0; p = p->next)
		if (dpkg_unpackcontrol(p) != 0)
		{
			perror(p->file);
			/* force loop break, and prevents further ops */
			pkgs = 0;
		}
	
	/* Stage 2: resolve dependencies */
#ifdef DODEPENDS
	ordered = depends_resolve(pkgs, status);
#else
	ordered = pkgs;
#endif
	
	/* Stage 3: install */			
	for (p = ordered; p != 0; p = p->next)
		if (dpkg_doinstall(p) != 0)
		{
			perror(p->file);
			break;
		}
	
	status_merge(status, pkgs);
	/*
	SYSTEM("rm -rf -- " DPKGCIDIR);
	*/
	return 0;
}

static int dpkg_remove(struct package_t *pkgs)
{
	struct package_t *p;
	void *status = status_read();
	for (p = pkgs; p != 0; p = p->next)
	{
	}
	/*
	status_merge(status, 0);
	*/
	return 0;
}

int main(int argc, char **argv)
{
	char opt = 0;
	struct package_t *p, *packages = NULL;
	char *cwd = getcwd(0, 0);
	while (*++argv)
	{
		if (**argv == '-')
			opt = *(*argv+1);
		else
		{
			p = (struct package_t *)malloc(sizeof(struct package_t));
			memset(p, 0, sizeof(struct package_t));
			if (**argv == '/')
				p->file = *argv;
			else
			{
				p->file = malloc(strlen(cwd) + strlen(*argv) + 2);
				sprintf(p->file, "%s/%s", cwd, *argv);
			}
			p->next = packages;
			packages = p;
		}
			
	}
	switch (opt)
	{
		case 'i': return dpkg_install(packages); break;
		case 'r': return dpkg_remove(packages); break;
	}

	/* if it falls through to here, some of the command line options were
	   wrong */
	printf("udpkg <-i|-r> my.deb\n");
	return 0;
}
