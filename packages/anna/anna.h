#ifndef ANNA_H_
#define ANNA_H_ 1

#include <cdebconf/debconfclient.h>
#include <debian-installer.h>
#include <stdbool.h>

#define RETRIEVER_DIR           "/usr/lib/debian-installer/retriever"
#define DOWNLOAD_DIR            "/var/cache/anna"
#define DOWNLOAD_PACKAGES       DOWNLOAD_DIR "/Packages"
#define INCLUDE_FILE            DOWNLOAD_DIR "/include"
#define EXCLUDE_FILE            DOWNLOAD_DIR "/exclude"
#define DPKG_UNPACK_COMMAND	"udpkg --unpack"
#define DPKG_CONFIGURE_COMMAND	"udpkg --configure"
#define ANNA_RETRIEVER          "anna/retriever"

#undef LIBDI_SYSTEM_DPKG

extern struct debconfclient *debconf;

di_package **get_retriever_packages(di_packages *status);
const char *get_default_retriever(const char *choices);
void set_retriever(const char *retriever);
char *get_retriever(void);
int config_retriever(void);
di_packages *get_packages(di_packages_allocator *allocator);
bool is_installed(di_package *p, di_packages *status);
size_t package_to_choice(di_package *package, char *buf, size_t size);
char *list_to_choices(di_package **packages);
int get_package (di_package *package, char *dest);
int md5sum(const char* sum, const char *file);
void cleanup(void);
int skip_package(di_package *p);
int package_array_compare(const void *v1, const void *v2);
void take_includes(di_packages *packages);
void drop_excludes(di_packages *packages);
int new_retrievers(di_package **retrievers_before, di_package **retrievers_after);
#ifndef LIBDI_SYSTEM_DPKG
int unpack_package (const char *pkgfile);
#endif
int configure_package (const char *package);
signed int retriever_handle_error (const char *failing_command);

#endif /* ANNA_H_ */
