#define BUFSIZE		4096
/* This should be set to 5 or so, set high because status_read is buggy. */
#define PACKAGECHUNK	100
#define STATUSFILE	"./status"

#define DEPENDSMAX	64	/* maximum number of depends we can handle */

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
