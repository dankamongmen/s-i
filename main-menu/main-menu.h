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
#define DPKGDIR 	"./var/lib/dpkg/"
#define STATUSFILE	DPKGDIR "status"

#define DEPENDSMAX	64	/* maximum number of depends we can handle */

#define MAIN_MENU	"debian-installer/main-menu"
#define DPKG_CONFIGURE_COMMAND "usr/bin/udpkg --configure"

typedef enum { unpacked, installed, other } package_status;

struct package_t {
	char *package;
	int installer_menu_item;
	char *description; /* short only, and only for menu items */
	char *depends[DEPENDSMAX];
	package_status status;
	int processed;
	struct package_t *next;
};

/* status.c */
struct package_t *status_read(void);

/* tree.c */
struct package_t *tree_find(char *);
struct package_t *tree_add(const char *);
void tree_clear();
