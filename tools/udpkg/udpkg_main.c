/* $Id: udpkg_main.c,v 1.1 2000/11/20 23:43:34 bug1 Exp $ */
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
#include <dirent.h>

#define UDPKG_CONFIGURE 1
#define UDPKG_INSTALL	2
#define UDPKG_REMOVE	4
#define UDPKG_UNPACK	8	

#ifdef UDPKG_MODULE
int udpkg(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	int opt, funct = 0;
	package_t *pkgs;

	while ((opt=getopt(argc, argv, "ciru")) != -1) {
		switch (opt) {
		case 'c':
			funct |= UDPKG_CONFIGURE;
			break;
		case 'i':
			funct |= UDPKG_INSTALL;
			break;
		case 'r':
			funct |= UDPKG_REMOVE;
			break;
		case 'u':
			funct |= UDPKG_UNPACK;
			break;
		default:
			printf("udpkg <-i|-r|-u|-c> my.deb\n");
			return(EXIT_FAILURE);
		}
	}

	/* Check that a package was specified */
	if (optind == argc) {
		printf("you didnt specify a package\n");
		return(EXIT_FAILURE);
	}

	pkgs = build_package_struct((argv+optind), funct);
	
	switch(funct) {
		case UDPKG_CONFIGURE: 
			dpkg_configure(pkgs);
			break;
		case UDPKG_INSTALL:
			dpkg_install(pkgs);
			break;
		case UDPKG_REMOVE:
			dpkg_remove(pkgs);
			break;
		case UDPKG_UNPACK:
			dpkg_unpack(pkgs);
			break;
		default:
	}
	return(EXIT_SUCCESS);
}
