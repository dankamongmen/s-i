#include "packages.h"

#define DOWNLOAD_DIR "/tmp"
#define DPKG_UNPACK_COMMAND "usr/bin/udpkg -u"

int get_package (char *src, char *dest);
struct package_t *get_packages (void);
