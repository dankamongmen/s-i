/* $Id: status.c,v 1.2 2000/10/08 03:23:44 tausq Exp $ */
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
		"half-configured", "installed", "half-installed",
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
			printf("corrupted status flag!!\n");
			return NULL;
		}
	}
	return buf;
}

void *status_read(void)
{
	FILE *f;
	char buf[BUFSIZE];
	void *status = 0;
	struct package_t *m = 0, *p = 0, *t = 0;

	if ((f = fopen(STATUSFILE, "r")) == NULL)
	{
		perror(STATUSFILE);
		return 0;
	}
	printf("(Reading database...)\n");
	while (fgets(buf, BUFSIZE, f) && !feof(f))
	{
		buf[strlen(buf)-1] = 0;
		if (*buf == 0)
		{
			if (m != 0)
			{
				tsearch(m, &status, package_compare);
			}

			m = 0;
		}
		else if (strstr(buf, "Package: ") == buf)
		{
			ASSERT(m == 0);
			m = (struct package_t *)malloc(sizeof(struct package_t));
			memset(m, 0, sizeof(struct package_t));
			m->package = strdup(buf+9);
			m->refcount = 1;
		} 
		else if (strstr(buf, "Status: ") == buf)
		{
			ASSERT(m != 0);
			m->status = status_parse(buf+8);
		}
#if 0
		else if (strstr(buf, "Depends: ") == buf)
		{
			ASSERT(m != 0);
			m->depends = strdup(buf+9);
		}
#endif
		else if (strstr(buf, "Provides: ") == buf)
		{
			/* a "Provides" triggers the insertion of a pseudo package
			 * into the status binary-tree
			 */
			ASSERT(m != 0);
			p = (struct package_t *)malloc(sizeof(struct package_t));
			memset(p, 0, sizeof(struct package_t));
			p->package = strdup(buf+10);
			p->status = m->status;
			t = *(struct package_t **)tsearch(p, &status, package_compare);
			if (t->refcount++ > 0)
			{
				free(p->package);
				free(p);
			}
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
	printf("(Updating database...)\n");
	while (fgets(buf, BUFSIZE, fin) && !feof(fin))
	{
		/* If we see a package header, find out if it's a package
		 * that we have processed. if so, we skip that block for 
		 * now (write it at the end. 
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
		if (pkg == 0) continue;

		if (strstr(buf, "Status: ") == buf && statpkg != 0)
		{
			  snprintf(buf, sizeof(buf), "Status: %s\n",
				 status_print(statpkg->status));
		}
		fputs(buf, fout);
	}
	fclose(fin);
	fclose(fout);

	r = rename(STATUSFILE, STATUSFILE ".bak");
	if (r == 0) r = rename(STATUSFILE ".new", STATUSFILE);
	return 0;
}
