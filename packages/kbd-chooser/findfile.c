#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "xmalloc.h"
#include "findfile.h"
#include "nls.h"

char pathname[1024];

#define SIZE(a) (sizeof(a)/sizeof(a[0]))

static FILE *
findfile_in_dir(char *fnam, char *dir, int recdepth, char **suf) {
	FILE *fp = NULL;
	DIR *d;
	struct dirent *de;
	struct stat sb;
	char *ff, *fdir, *p, *q, **sp;
	int secondpass = 0;

	ff = index(fnam, '/');
	if (ff) {
		fdir = xstrdup(fnam);
		fdir[ff-fnam] = 0; 	/* caller guarantees fdir != "" */
	} else
		fdir = 0;		/* just to please gcc */

	/* Scan the directory twice: first for files, then
	   for subdirectories, so that we do never search
	   a subdirectory when the directory itself already
	   contains the file we are looking for. */
 StartScan:
	d = opendir(dir);
	if (d == NULL)
	    return NULL;
	while ((de = readdir(d)) != NULL) {
	    int okdir;

	    if (strcmp(de->d_name, ".") == 0 ||
		strcmp(de->d_name, "..") == 0)
		continue;

	    if (strlen(dir) + strlen(de->d_name) + 2 > sizeof(pathname))
		continue;

	    okdir = (ff && strcmp(de->d_name, fdir) == 0);

	    if ((secondpass && recdepth) || okdir) {
       		struct stat statbuf;
		char *a;

		a = xmalloc(strlen(dir) + strlen(de->d_name) + 2);
		sprintf(a, "%s/%s", dir, de->d_name);
		if (stat(a, &statbuf) == 0 &&
		    S_ISDIR(statbuf.st_mode)) {
			if (okdir)
				fp = findfile_in_dir(ff+1, a, 0, suf);
			if (!fp && recdepth)
				fp = findfile_in_dir(fnam, a, recdepth-1, suf);
			if (fp)
				return fp;
		}
		free(a);
	    }

	    if (secondpass)
		    continue;

	    /* Should we be in a subdirectory? */
	    if (ff)
		    continue;

	    /* Does d_name start right? */
	    p = &de->d_name[0];
	    q = fnam;
	    while (*p && *p == *q) p++,q++;
	    if (*q)
		    continue;

	    sprintf(pathname, "%s/%s", dir, de->d_name);

	    /* Does tail consist of a known suffix and possibly
	       a compression suffix? */
	    for(sp = suf; *sp; sp++) {
		    if (!strcmp(p, *sp))  {
			    if (stat(pathname, &sb)==0 && S_ISDIR(sb.st_mode))
				    continue;
		      return fopen (pathname, "r");
		    }
	    }
	}
	closedir(d);
	if (recdepth > 0 && !secondpass) {
		secondpass = 1;
		goto StartScan;
	}
	return NULL;
}

/* find input file; leave name in pathname[] */
FILE *findfile(char *fnam, char **dirpath, char **suffixes) {
        char **dp, *dir, **sp;
	FILE *fp;
	int dl, recdepth;

	if (strlen(fnam) >= sizeof(pathname))
		return NULL;

	/* Try explicitly given name first */
	strcpy(pathname, fnam);
	// fp = maybe_pipe_open();
	fp = fopen (pathname, "r");
	if (fp)
		return fp;

	/* Test for full pathname - opening it failed, so need suffix */
	/* (This is just nonsense, for backwards compatibility.) */
	if (*fnam == '/') {
	    for (sp = suffixes; *sp; sp++) {
		if (strlen(fnam) + strlen(*sp) + 1 > sizeof(pathname))
		    continue;
		if (*sp == 0)
		    continue;	/* we tried it already */
		sprintf(pathname, "%s%s", fnam, *sp);
		if((fp = fopen(pathname, "r")) != NULL)
		    return fp;
	    }
	    return NULL;
	}

	/* Search a list of directories and directory hierarchies */
	for (dp = dirpath; *dp; dp++) {

	    /* delete trailing slashes; trailing stars denote recursion */
	    dir = xstrdup(*dp);
	    dl = strlen(dir);
	    recdepth = 0;
	    while (dl && dir[dl-1] == '*') {
		    dir[--dl] = 0;
		    recdepth++;
	    }
	    if (dl == 0) {
		    dir = ".";
	    } else if (dl > 1 && dir[dl-1] == '/') {
		    dir[dl-1] = 0;
	    }

	    fp = findfile_in_dir(fnam, dir, recdepth, suffixes);
	    if (fp)
		    return fp;
	}

	return NULL;
}
