/* $Id: udpkg.h,v 1.7 2000/11/20 23:43:34 bug1 Exp $ */
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
#define STATUS_STATUSPOSTINSTFAILED	(1 << 15)
#define STATUS_STATUSREMOVALFAILED	(1 << 16)
#define STATUS_STATUSMASK		~(STATUS_STATUSNOTINSTALLED | STATUS_STATUSUNPACKED | STATUS_STATUSHALFCONFIGURED | STATUS_STATUSCONFIGFILES | STATUS_STATUSPOSTINSTFAILED | STATUS_STATUSREMOVALFAILED)

#define COLOR_WHITE			0
#define COLOR_GRAY			1
#define COLOR_BLACK			2

#define UDPKG_CONFIGURE 1
#define UDPKG_INSTALL   2   
#define UDPKG_REMOVE    4
#define UDPKG_UNPACK    8

/* data structures */
typedef struct package_s {
	char *file;
	char *package;
	char *version;
	char *depends;
	char *provides;
	char *description;
	int installer_menu_item;
	unsigned long status;
	char color; /* for topo-sort */
	struct package_s *requiredfor[DEPENDSMAX]; 
	unsigned short requiredcount;
	struct package_s *next;
} package_t;

/* function prototypes */
void *status_read(void);
void control_read(FILE *f, package_t *p);
int status_merge(void *status, package_t *pkgs);
int package_compare(const void *p1, const void *p2);
package_t *depends_resolve(package_t *pkgs, void *status);
extern package_t *build_package_struct(const char **name_list, int funct);
extern int dpkg_unpack(package_t *pkgs);
extern int dpkg_configure(package_t *pkgs);
extern int dpkg_install(package_t *pkgs);
extern int dpkg_remove(package_t *pkgs);

#endif
