#include "packages.h"

#define DOWNLOAD_DIR "var/cache/anna"
#define DPKG_UNPACK_COMMAND "usr/bin/udpkg --unpack"

int get_package (struct package_t *package, char *dest);
struct package_t *get_packages (void);
