#ifndef _DEBIAN_INSTALLER_H_
#define _DEBIAN_INSTALLER_H_
/* 
   debian-installer.h - common utilities for debian-installer
*/

#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>

#define MAXLINE 512

#define DEPENDSMAX	64	/* maximum number of depends we can handle */
#define PROVIDESMAX     16      /* maximum number of provides we can handle */
typedef enum { unpacked, installed, half_configured, other } package_status;
typedef enum { extra, optional, standard, important, required } package_priority;

struct language_description
{
        char *language;
        char *description;
        char *extended_description;
        struct language_description *next;
};

struct package_t {
        char *package;
	char *filename;
	char *md5sum;
        int installer_menu_item;
        char *description; /* short only, and only for menu items */
        char *depends[DEPENDSMAX];
        char *provides[PROVIDESMAX];
        package_status status;
        int processed;
        struct language_description *localized_descriptions;
        struct package_t *next;
        package_priority priority;
	char *version;
};

struct version_t {
    unsigned long epoch;
    const char *version;
    const char *revision;
};

int di_prebaseconfig_append (const char *udeb, const char *format, ...);
int di_execlog (const char *incmd);
void di_log(const char *msg);
int di_check_dir (const char *dirname);
int di_snprintfcat (char *str, size_t size, const char *format, ...);
char *di_stristr(const char *haystack, const char *needle);
struct package_t *di_pkg_parse(FILE *fp);
int di_parse_version(struct version_t *rversion, const char *string);
int di_compare_version(const struct version_t *a, const struct version_t *b);

#endif /* _DEBIAN_INSTALLER_H_ */
