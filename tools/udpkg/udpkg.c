/* $Id: udpkg.c,v 1.23 2000/12/08 06:25:40 joeyh Rel $ */
#include "udpkg.h"

#include <errno.h>
#include <fcntl.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>

/* 
 * Main udpkg implementation routines
 */

#ifdef DODEBUG
static int do_system(const char *cmd)
{
	DPRINTF("cmd is %s\n", cmd);
	return system(cmd);
}
#else
#define do_system(cmd) system(cmd)
#endif

static int is_file(const char *fn)
{
	struct stat statbuf;

	if (stat(fn, &statbuf) < 0) return 0;
	return S_ISREG(statbuf.st_mode);
}

static int dpkg_copyfile(const char *src, const char *dest)
{
	/* copy a (regular) file if it exists, preserving the mode, mtime 
	 * and atime */
	char buf[8192];
	int infd, outfd;
	int r;
	struct stat srcStat;
	struct utimbuf times;

	if (stat(src, &srcStat) < 0) 
	{
		if (errno == 2) return 0; else return -1;
	}
	if ((infd = open(src, O_RDONLY)) < 0) 
		return -1;
	if ((outfd = open(dest, O_WRONLY|O_CREAT|O_TRUNC, srcStat.st_mode)) < 0)
		return -1;
	while ((r = read(infd, buf, sizeof(buf))) > 0)
	{
		if (write(outfd, buf, r) < 0)
			return -1;
	}
	close(outfd);
	close(infd);
	if (r < 0) return -1;
	times.actime = srcStat.st_atime;
	times.modtime = srcStat.st_mtime;
	if (utime(dest, &times) < 0) return -1;
	return 1;
}

static int dpkg_doconfigure(struct package_t *pkg)
{
	int r;
	char postinst[1024];
	char buf[1024];
	DPRINTF("Configuring %s\n", pkg->package);
	pkg->status &= STATUS_STATUSMASK;
	snprintf(postinst, sizeof(postinst), "%s%s.postinst", INFODIR, pkg->package);
	if (is_file(postinst))
	{
		snprintf(buf, sizeof(buf), "%s configure", postinst);
		if ((r = do_system(buf)) != 0)
		{
			fprintf(stderr, "postinst exited with status %d\n", r);
			pkg->status |= STATUS_STATUSHALFCONFIGURED;
			return 1;
		}
	}

	pkg->status |= STATUS_STATUSINSTALLED;
	
	return 0;
}

static int dpkg_dounpack(struct package_t *pkg)
{
	int r = 0;
	char *cwd, *p;
	FILE *infp, *outfp;
	char buf[1024], buf2[1024];
	int i;
	char *adminscripts[] = { "prerm", "postrm", "preinst", "postinst",
	                         "conffiles", "md5sums", "shlibs", 
				 "templates" };

	DPRINTF("Unpacking %s\n", pkg->package);

	cwd = getcwd(0, 0);
	/* chdir("/"); */
	//chdir("tmp/"); /* testing */
	snprintf(buf, sizeof(buf), "ar -p %s data.tar.gz|zcat|tar -xf -", pkg->file);
	if (SYSTEM(buf) == 0)
	{
		/* Installs the package scripts into the info directory */
		for (i = 0; i < sizeof(adminscripts) / sizeof(adminscripts[0]);
		     i++)
		{
			snprintf(buf, sizeof(buf), "%s%s/%s",
				DPKGCIDIR, pkg->package, adminscripts[i]);
			snprintf(buf2, sizeof(buf), "%s%s.%s", 
				INFODIR, pkg->package, adminscripts[i]);
			if (dpkg_copyfile(buf, buf2) < 0)
			{
				fprintf(stderr, "Cannot copy %s to %s: %s\n", 
					buf, buf2, strerror(errno));
				r = 1;
				break;
			}
			else
			{
				/* ugly hack to create the list file; should
				 * probably do something more elegant
				 *
				 * why oh why does dpkg create the list file
				 * so oddly...
				 */
				snprintf(buf, sizeof(buf), 
					"ar -p %s data.tar.gz|zcat|tar -tf -", 
					pkg->file);
				snprintf(buf2, sizeof(buf2),
					"%s%s.list", INFODIR, pkg->package);
				if ((infp = popen(buf, "r")) == NULL ||
				    (outfp = fopen(buf2, "w")) == NULL)
				{
					fprintf(stderr, "Cannot create %s\n",
						buf2);
					r = 1;
					break;
				}
				while (fgets(buf, sizeof(buf), infp) &&
				       !feof(infp))
				{
					p = buf;
					if (*p == '.') p++;
					if (*p == '/' && *(p+1) == '\n')
					{
						*(p+1) = '.';
						*(p+2) = '\n';
						*(p+3) = 0;
					}
					if (p[strlen(p)-2] == '/')
					{
						p[strlen(p)-2] = '\n';
						p[strlen(p)-1] = 0;
					}
					fputs(p, outfp);
				}
				fclose(infp);
				fclose(outfp);
			}
		}
		pkg->status &= STATUS_WANTMASK;
		pkg->status |= STATUS_WANTINSTALL;
		pkg->status &= STATUS_FLAGMASK;
		pkg->status |= STATUS_FLAGOK;
		pkg->status &= STATUS_STATUSMASK;
		if (r == 0)
			pkg->status |= STATUS_STATUSUNPACKED;
		else
			pkg->status |= STATUS_STATUSHALFINSTALLED;
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
	char buf[1024];
	FILE *f;

	p = strrchr(pkg->file, '/');
	if (p) p++; else p = pkg->file;
	p = pkg->package = strdup(p);
	while (*p != 0 && *p != '_' && *p != '.') p++;
	*p = 0;

	cwd = getcwd(0, 0);
	snprintf(buf, sizeof(buf), "%s%s", DPKGCIDIR, pkg->package);
	DPRINTF("dir = %s\n", buf);
	if (mkdir(buf, S_IRWXU) == 0 && chdir(buf) == 0)
	{
		snprintf(buf, sizeof(buf), "ar -p %s control.tar.gz|zcat|tar -xf -",
			pkg->file);
		if (SYSTEM(buf) == 0)
		{
			if ((f = fopen("control", "r")) != NULL) {
				control_read(f, pkg);
				r = 0;
			}
		}
	}

	chdir(cwd);
	free(cwd);
	return r;
}

static int dpkg_unpack(struct package_t *pkgs)
{
	int r = 0;
	struct package_t *pkg;
	void *status = status_read();

	if (SYSTEM("rm -rf -- " DPKGCIDIR) != 0 ||
	    mkdir(DPKGCIDIR, S_IRWXU) != 0)
	{
		perror("mkdir");
		return 1;
	}
	
	for (pkg = pkgs; pkg != 0; pkg = pkg->next)
	{
		dpkg_unpackcontrol(pkg);
		r = dpkg_dounpack(pkg);
		if (r != 0) break;
	}
	status_merge(status, pkgs);
	SYSTEM("rm -rf -- " DPKGCIDIR);
	return r;
}

static int dpkg_configure(struct package_t *pkgs)
{
	int r = 0;
	void *found;
	struct package_t *pkg;
	void *status = status_read();
	for (pkg = pkgs; pkg != 0 && r == 0; pkg = pkg->next)
	{
		found = tfind(pkg, &status, package_compare);
		if (found == 0)
		{
			fprintf(stderr, "Trying to configure %s, but it is not installed\n", pkg->package);
			r = 1;
		}
		else
		{
			/* configure the package listed in the status file;
			 * not pkg, as we have info only for the latter */
			r = dpkg_doconfigure(*(struct package_t **)found);
		}
	}
	status_merge(status, 0);
	return r;
}

static int dpkg_install(struct package_t *pkgs)
{
	struct package_t *p, *ordered = 0;
	void *status = status_read();
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
	{
		p->status &= STATUS_WANTMASK;
		p->status |= STATUS_WANTINSTALL;

		/* for now the flag is always set to ok... this is probably
		 * not what we want
		 */
		p->status &= STATUS_FLAGMASK;
		p->status |= STATUS_FLAGOK;

		if (dpkg_doinstall(p) != 0)
		{
			perror(p->file);
		}
	}
	
	if (ordered != 0)
		status_merge(status, pkgs);
	SYSTEM("rm -rf -- " DPKGCIDIR);
	return 0;
}

static int dpkg_remove(struct package_t *pkgs)
{
	struct package_t *p;
	void *status = status_read();
	for (p = pkgs; p != 0; p = p->next)
	{
	}
	status_merge(status, 0);
	return 0;
}

#ifdef UDPKG_MODULE
int udpkg(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	char opt = 0;
	char *s;
	struct package_t *p, *packages = NULL;
	char *cwd = getcwd(0, 0);
	while (*++argv)
	{
		if (**argv == '-') {
			/* Nasty little hack to "parse" long options. */
			s = *argv;
			while (*s == '-')
				s++;
			opt=s[0];
		}
		else
		{
			p = (struct package_t *)malloc(sizeof(struct package_t));
			memset(p, 0, sizeof(struct package_t));
			if (**argv == '/')
				p->file = *argv;
			else if (opt != 'c')
			{
				p->file = malloc(strlen(cwd) + strlen(*argv) + 2);
				sprintf(p->file, "%s/%s", cwd, *argv);
			}
			else {
				p->package = strdup(*argv);
			}
			p->next = packages;
			packages = p;
		}
			
	}
	switch (opt)
	{
		case 'i': return dpkg_install(packages); break;
		case 'r': return dpkg_remove(packages); break;
		case 'u': return dpkg_unpack(packages); break;
		case 'c': return dpkg_configure(packages); break;
	}

	/* if it falls through to here, some of the command line options were
	   wrong */
	fprintf(stderr, "udpkg <-i|-r|--unpack|--configure> my.deb\n");
	return 0;
}
