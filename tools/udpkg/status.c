/* $Id: status.c,v 1.18 2000/12/08 05:59:10 joeyh Rel $ */
#include "udpkg.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <search.h>

/* Status file handling routines
 * 
 * This is a fairly minimalistic implementation. there are two main functions 
 * that are supported:
 * 
 * 1) reading the entire status file:
 *    the status file is read into memory as a binary-tree, with just the 
 *    package and status info preserved
 *
 * 2) merging the status file
 *    control info from (new) packages is merged into the status file, 
 *    replacing any pre-existing entries. when a merge happens, status info 
 *    read using the status_read function is written back to the status file
 */

static const char *statuswords[][10] = {
	{ (char *)STATUS_WANTSTART, "unknown", "install", "hold", 
		"deinstall", "purge", 0 },
	{ (char *)STATUS_FLAGSTART, "ok", "reinstreq", "hold", 
		"hold-reinstreq", 0 },
	{ (char *)STATUS_STATUSSTART, "not-installed", "unpacked", 
		"installed", "half-installed",
		"config-files", "post-inst-failed", 
		"removal-failed", 0 }
};

int package_compare(const void *p1, const void *p2)
{
	return strcmp(((struct package_t *)p1)->package, 
		((struct package_t *)p2)->package);
}

static unsigned long status_parse(const char *line)
{
	char *p;
	int i, j;
	unsigned long l = 0;
	for (i = 0; i < 3; i++)
	{
		p = strchr(line, ' ');
		if (p) *p = 0; 
		j = 1;
		while (statuswords[i][j] != 0) 
		{
			if (strcmp(line, statuswords[i][j]) == 0) 
			{
				l |= (1 << ((int)statuswords[i][0] + j - 1));
				break;
			}
			j++;
		}
		if (statuswords[i][j] == 0) return 0; /* parse error */
		line = p+1;
	}
	return l;
}

static const char *status_print(unsigned long flags)
{
	/* this function returns a static buffer... */
	static char buf[256];
	int i, j;

	buf[0] = 0;
	for (i = 0; i < 3; i++)
	{
		j = 1;
		while (statuswords[i][j] != 0)
		{
			if ((flags & (1 << ((int)statuswords[i][0] + j - 1))) != 0)
			{
				strcat(buf, statuswords[i][j]);
				if (i < 2) strcat(buf, " ");
				break;
			}
			j++;
		}
		if (statuswords[i][j] == 0)
		{
			fprintf(stderr, "corrupted status flag!!\n");
			return NULL;
		}
	}
	return buf;
}

/*
 * Read a control file (or a stanza of a status file) and parse it,
 * filling parsed fields into the package structure
 */
void control_read(FILE *f, struct package_t *p)
{
	char buf[BUFSIZE];
	while (fgets(buf, BUFSIZE, f) && !feof(f))
	{
		buf[strlen(buf)-1] = 0;
		if (*buf == 0)
			return;
		else if (strstr(buf, "Package: ") == buf)
		{
			p->package = strdup(buf+9);
		}
		else if (strstr(buf, "Status: ") == buf)
		{
			p->status = status_parse(buf+8);
		}
		else if (strstr(buf, "Depends: ") == buf)
		{
			p->depends = strdup(buf+9);
		}
		else if (strstr(buf, "Provides: ") == buf)
		{
			p->provides = strdup(buf+10);
		}
		/* This is specific to the Debian Installer. Ifdef? */
		else if (strstr(buf, "installer-menu-item: ") == buf) 
		{
			p->installer_menu_item = atoi(buf+21);
		}
		else if (strstr(buf, "Description: ") == buf)
		{
			p->description = strdup(buf+13);
		}
		/* TODO: localized descriptions */
	}
}

void *status_read(void)
{
	FILE *f;
	void *status = 0;
	struct package_t *m = 0, *p = 0, *t = 0;

	if ((f = fopen(STATUSFILE, "r")) == NULL)
	{
		perror(STATUSFILE);
		return 0;
	}
	if (getenv(UDPKG_QUIET) == NULL)
		printf("(Reading database...)\n");
	while (!feof(f))
	{
		m = (struct package_t *)malloc(sizeof(struct package_t));
		memset(m, 0, sizeof(struct package_t));
		control_read(f, m);
		if (m->package)
		{
			tsearch(m, &status, package_compare);
			if (m->provides)
			{
				/* 
				 * A "Provides" triggers the insertion
				 * of a pseudo package into the status
				 * binary-tree.
				 */
				p = (struct package_t *)malloc(sizeof(struct package_t));
				memset(p, 0, sizeof(struct package_t));
				p->package = strdup(m->provides);
				t = *(struct package_t **)tsearch(p, &status, package_compare);
				if (!(t == p))
				{
					free(p->package);
					free(p);
				}
				else {
					/*
					 * Pseudo package status is the
					 * same as the status of the
					 * package providing it 
					 * FIXME: (not quite right, if 2
					 * packages of different statuses
					 * provide it).
					 */
					t->status = m->status;
				}
			}
		}
		else
		{
			free(m);
		}
	}
	fclose(f);
	return status;
}

int status_merge(void *status, struct package_t *pkgs)
{
	FILE *fin, *fout;
	char buf[BUFSIZE];
	struct package_t *pkg = 0, *statpkg = 0;
	struct package_t locpkg;
	int r = 0;

	if ((fin = fopen(STATUSFILE, "r")) == NULL)
	{
		perror(STATUSFILE);
		return 0;
	}
	if ((fout = fopen(STATUSFILE ".new", "w")) == NULL)
	{
		perror(STATUSFILE ".new");
		return 0;
	}
	if (getenv(UDPKG_QUIET) == NULL)
		printf("(Updating database...)\n");
	while (fgets(buf, BUFSIZE, fin) && !feof(fin))
	{
		buf[strlen(buf)-1] = 0; /* trim newline */
		/* If we see a package header, find out if it's a package
		 * that we have processed. if so, we skip that block for
		 * now (write it at the end).
		 *
		 * we also look at packages in the status cache and update
		 * their status fields
		 */
		if (strstr(buf, "Package: ") == buf)
		{
			for (pkg = pkgs; pkg != 0 && strncmp(buf+9,
					pkg->package, strlen(pkg->package))!=0;
			     pkg = pkg->next) ;

			locpkg.package = buf+9;
			statpkg = tfind(&locpkg, &status, package_compare);
			
			/* note: statpkg should be non-zero, unless the status
			 * file was changed while we are processing (no locking
			 * is currently done...
			 */
			if (statpkg != 0) statpkg = *(struct package_t **)statpkg;
		}
		if (pkg != 0) continue;

		if (strstr(buf, "Status: ") == buf && statpkg != 0)
		{
			  snprintf(buf, sizeof(buf), "Status: %s",
				 status_print(statpkg->status));
		}
		fputs(buf, fout);
		fputc('\n', fout);
	}

	// Print out packages we processed.
	for (pkg = pkgs; pkg != 0; pkg = pkg->next) {
		fprintf(fout, "Package: %s\nStatus: %s\n", 
			pkg->package, status_print(pkg->status));
		if (pkg->depends)
			fprintf(fout, "Depends: %s\n", pkg->depends);
		if (pkg->provides)
			fprintf(fout, "Provides: %s\n", pkg->provides);
		if (pkg->installer_menu_item)
			fprintf(fout, "installer-menu-item: %i\n", pkg->installer_menu_item);
		if (pkg->description)
			fprintf(fout, "Description: %s\n", pkg->description);
		fputc('\n', fout);
	}
	
	fclose(fin);
	fclose(fout);

	r = rename(STATUSFILE, STATUSFILE ".bak");
	if (r == 0) r = rename(STATUSFILE ".new", STATUSFILE);
	return 0;
}
