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
#define DPKGDIR 	"./"	// "/var/lib/dpkg/"
#define STATUSFILE	DPKGDIR "status"

#define DEPENDSMAX	64	/* maximum number of depends we can handle */

#define MAIN_MENU	"debian-installer/main-menu"

#define STATUS_UNKNOWN		0
#define STATUS_UNPACKED		1
#define STATUS_INSTALLED	2

/* data structures */
struct package_t {
	char *package;
	int installer_menu_item;
	char *description; /* short only, and only for menu items */
	char *description_ll;
	char *depends[DEPENDSMAX];
	int status;
	int processed;
	struct package_t *next;
};

/* status.c */
struct package_t *status_read(void);

/* debconf.c */
int debconf_command (const char *, ...);
char *debconf_ret (void);

/* tree.c */
struct package_t *tree_find(char *);
struct package_t *tree_add(const char *);
void tree_clear();
