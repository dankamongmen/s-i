#include <sys/types.h>


typedef struct {
        u_int32_t long_id;
} pci_ids_t;


typedef struct {
        char *name;
        pci_ids_t *id;
} pci_module_list_t;


typedef struct {
       char *dev_id;
} isa_dev_id_t;

typedef struct {
    char *name;
    isa_dev_id_t *id;
} isa_module_list_t;


extern int passive_detection;
void ddetect_getopts(int argc, char *argv[]);
