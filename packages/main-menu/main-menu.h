#ifdef DODEBUG
#include <assert.h>
#define ASSERT(x) assert(x)
#define DPRINTF(fmt,args...) fprintf(stderr, fmt, ##args)
#else
#define ASSERT(x) /* nothing */
#define DPRINTF(fmt,args...) /* nothing */
#endif

#define BUFSIZE		4096

#define MAIN_MENU	"debian-installer/main-menu"
#define MISSING_PROVIDE "debian-installer/missing-provide"
#define ITEM_FAILURE	"debian-installer/main-menu/item-failure"

#include <debian-installer.h>

#define NEVERDEFAULT 900

#define EXIT_OK 	        0
#define EXIT_BACKUP	        10
#define EXIT_QUIT	        11
#define EXIT_RESTART	        12
