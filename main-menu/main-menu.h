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
	char *depends[DEPENDSMAX];
	int status;
	int processed;
	struct package_t *next;
};

struct package_t *status_read(void);
int package_compare (const void *, const void *);
int debconf_command (const char *, ...);
