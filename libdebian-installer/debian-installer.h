#ifndef _DEBIAN_INSTALLER_H_
#define _DEBIAN_INSTALLER_H_
/* 
   debian-installer.h - common utilities for debian-installer
*/

#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>

#define DPKG_CONFIGURE_COMMAND "/usr/bin/udpkg --configure"
#define DPKGDIR         "/var/lib/dpkg/"
#define STATUSFILE      DPKGDIR "status"

#define MAXLINE 512

#define DEPENDSMAX	64	/* maximum number of depends we can handle */
#define RECOMMENDSMAX	64	/* maximum number of recommends we can handle */
#define PROVIDESMAX     16      /* maximum number of provides we can handle */
#define ENHANCESMAX     16      /* maximum number of enhances we can handle */
typedef enum { unpacked, installed, half_configured, other } package_status;
typedef enum { extra, optional, standard, important, required } package_priority;

struct language_description
{
        char *language;
        char *description;
        char *extended_description;
        struct language_description *next;
};

struct package_dependency
{
    char *name;
    struct package_t *ptr;
};

struct package_t {
        char *package;
	char *filename;
	char *md5sum;
        int installer_menu_item;
        char *description; /* short only, and only for menu items */
        struct package_dependency *depends[DEPENDSMAX];
        struct package_dependency *provides[PROVIDESMAX];
        package_status status;
        int processed;
        struct language_description *localized_descriptions;
        package_priority priority;
	char *version;
	struct package_dependency *recommends[RECOMMENDSMAX];
	struct package_dependency *enhances[ENHANCESMAX];
};

struct version_t {
    unsigned long epoch;
    const char *version;
    const char *revision;
};

struct list_node
{
    struct list_node *next;
    void *data;
};

struct linkedlist_t
{
    struct list_node *head;
    struct list_node *tail;
};

int di_prebaseconfig_append (const char *udeb, const char *format, ...);
int di_execlog (const char *incmd);
void di_log(const char *msg);
void di_logf(const char *fmt, ...);
int di_check_dir (const char *dirname);
int di_snprintfcat (char *str, size_t size, const char *format, ...);
char *di_stristr(const char *haystack, const char *needle);
struct package_t *di_pkg_alloc(const char *name);
void di_pkg_free(struct package_t *p);
struct linkedlist_t *di_pkg_parse(FILE *fp);
struct linkedlist_t *di_status_read(void);
struct package_t *di_pkg_find(struct linkedlist_t *ptr, const char *package);
int di_pkg_provides(struct package_t *p, struct package_t *target);
int di_pkg_is_virtual(struct package_t *p);
int di_pkg_is_installed(struct package_t *p);
void di_pkg_resolve_deps(struct linkedlist_t *ptr);
struct linkedlist_t *di_pkg_toposort_arr(struct package_t **packages, const int pkg_count);
struct linkedlist_t *di_pkg_toposort_list(struct linkedlist_t *list);
int di_parse_version(struct version_t *rversion, const char *string);
int di_compare_version(const struct version_t *a, const struct version_t *b);
void di_list_free(struct linkedlist_t *list, void (*freefunc)(void *));
ssize_t di_mapdevfs(const char *path, char *ret, size_t len);
int di_config_package(struct package_t *p,
                      int (*virtfunc)(struct package_t *),
                      int (*walkfunc)(struct package_t *));

#endif /* _DEBIAN_INSTALLER_H_ */
