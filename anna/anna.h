#include <debian-installer.h>

#define DOWNLOAD_DIR "/var/cache/anna"
#define DPKG_UNPACK_COMMAND "udpkg --unpack"

#define ANNA_RETRIEVER			"anna/retriever"
#define ANNA_CHOOSE_LOWPRI_PACKAGES	"anna/choose_lowpri_packages"
#define RETRIEVER_DIR	"/usr/lib/debian-installer/retriever"

#define STATUS_FILE "/var/lib/dpkg/status"

int get_package (struct package_t *package, char *dest);
struct package_t *get_packages (void);
void cleanup (void);
