#include <cdebconf/debconfclient.h>
#include <debian-installer.h>
#include <stdbool.h>

#define LOWMEM_STATUS_FILE	"/var/lib/lowmem"
#define RETRIEVER_DIR		"/usr/lib/debian-installer/retriever"
#define DOWNLOAD_DIR		"/var/cache/anna"
#define DOWNLOAD_PACKAGES	DOWNLOAD_DIR "/Packages"
#define DOWNLOAD_PACKAGES_DEFAULT	DOWNLOAD_DIR "/Packages.default"
#define INCLUDE_FILE		DOWNLOAD_DIR "/include"
#define EXCLUDE_FILE		DOWNLOAD_DIR "/exclude"
#define DPKG_UNPACK_COMMAND	"udpkg --unpack"
#define DPKG_CONFIGURE_COMMAND	"udpkg --configure"

extern struct debconfclient *debconf;
