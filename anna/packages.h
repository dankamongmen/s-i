#define BUFSIZE		4096

struct package_t {
	char *package;
	char *filename;
	char *md5sum;
	int installer_menu_item;
	struct package_t *next;
};
