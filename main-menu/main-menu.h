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
#define DPKGDIR 	"/var/lib/dpkg/"
#define STATUSFILE	DPKGDIR "status"

#define MAIN_MENU	"debian-installer/main-menu"
#define MISSING_PROVIDE "debian-installer/missing-provide"
#define DPKG_CONFIGURE_COMMAND "/usr/bin/udpkg --configure"

#include <debian-installer.h>

/* status.c */
struct linkedlist_t *status_read(void);

