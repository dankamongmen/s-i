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

#include <debian-installer.h>

