/* $Id: udpkg.h,v 1.10 2000/12/08 05:59:10 joeyh Rel $ */
#ifndef _UDPKG_H_
#define _UDPKG_H_

#include <stdio.h>

#include "config.h"

#ifdef DODEBUG
#include <assert.h>
#define ASSERT(x) assert(x)
#define SYSTEM(x) do_system(x)
#define DPRINTF(fmt,args...) fprintf(stderr, fmt, ##args)
#else
#define ASSERT(x) /* nothing */
#define SYSTEM(x) system(x)
#define DPRINTF(fmt,args...) /* nothing */
#endif

#define BUFSIZE		4096
#define STATUSFILE	ADMINDIR ## "/status"
#define DPKGCIDIR	ADMINDIR ## "/tmp.ci/"
#define INFODIR		ADMINDIR ## "/info/"
#define UDPKG_QUIET	"UDPKG_QUIET"
#define DEPENDSMAX	64	/* maximum number of depends we can handle */

#define STATUS_WANTSTART		(0)
#define STATUS_WANTUNKNOWN		(1 << 0)
#define STATUS_WANTINSTALL		(1 << 1)
#define STATUS_WANTHOLD			(1 << 2)
#define STATUS_WANTDEINSTALL		(1 << 3)
#define STATUS_WANTPURGE		(1 << 4)
#define STATUS_WANTMASK			~(STATUS_WANTUNKNOWN | STATUS_WANTINSTALL | STATUS_WANTHOLD | STATUS_WANTDEINSTALL | STATUS_WANTPURGE)

#define STATUS_FLAGSTART		(5)
#define STATUS_FLAGOK			(1 << 5)
#define STATUS_FLAGREINSTREQ		(1 << 6)
#define STATUS_FLAGHOLD			(1 << 7)
#define STATUS_FLAGHOLDREINSTREQ	(1 << 8)
#define STATUS_FLAGMASK			~(STATUS_FLAGOK | STATUS_FLAGREINSTREQ | STATUS_FLAGHOLD | STATUS_FLAGHOLDREINSTREQ)

#define STATUS_STATUSSTART		(9)
#define STATUS_STATUSNOTINSTALLED	(1 << 9)
#define STATUS_STATUSUNPACKED		(1 << 10)
#define STATUS_STATUSHALFCONFIGURED	(1 << 11)
#define STATUS_STATUSINSTALLED		(1 << 12)
#define STATUS_STATUSHALFINSTALLED	(1 << 13)
#define STATUS_STATUSCONFIGFILES	(1 << 14)
#define STATUS_STATUSREMOVALFAILED	(1 << 16)
#define STATUS_STATUSMASK		~(STATUS_STATUSNOTINSTALLED | STATUS_STATUSUNPACKED | STATUS_STATUSHALFCONFIGURED | STATUS_STATUSCONFIGFILES | STATUS_STATUSREMOVALFAILED)

#define COLOR_WHITE			0
#define COLOR_GRAY			1
#define COLOR_BLACK			2

/* data structures */
struct package_t {
	char *file;
	char *package;
	char *version;
	char *depends;
	char *provides;
	char *description;
	int installer_menu_item;
	unsigned long status;
	char color; /* for topo-sort */
	struct package_t *requiredfor[DEPENDSMAX]; 
	unsigned short requiredcount;
	struct package_t *next;
};

/* function prototypes */
void *status_read(void);
void control_read(FILE *f, struct package_t *p);
int status_merge(void *status, struct package_t *pkgs);
int package_compare(const void *p1, const void *p2);
struct package_t *depends_resolve(struct package_t *pkgs, void *status);

#endif
