#define BUFSIZE		4096
#define STATUSFILE "/var/lib/dpkg/status"

#define DEPENDSMAX	64	/* maximum number of depends we can handle */

#define COLOR_WHITE	0
#define COLOR_GRAY	1
#define COLOR_BLACK	2

/* data structures */
struct package_t {
	char *package;
	char *provides;
	int installer_menu_item;
	char *description; /* short only */
	char color; /* for topo-sort */
	char refcount;
	struct package_t *requiredfor[DEPENDSMAX];
	unsigned short requiredcount;
	struct package_t *next;
};

int package_compare(const void *p1, const void *p2);
void *status_read(void);

