#define BUFSIZE		4096
#define STATUSFILE	"./status"

#define DEPENDSMAX	64	/* maximum number of depends we can handle */

#define MAIN_MENU	"debian-installer/main-menu"

/* data structures */
struct package_t {
	char *package;
	int installer_menu_item;
	char *description; /* short only, and only for menu items */
	char *depends[DEPENDSMAX];
	int processed;
	struct package_t *next;
};

struct package_t *status_read(void);
int package_compare (const void *, const void *);
int debconf_command (char *);
