/*
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pci/pci.h>

typedef struct pci_ids_s {
	u32 vendor;
	u32 device;
} pci_ids_t;

typedef struct module_list_s {
	char *name;
	pci_ids_t *id;	
} module_list_t;

typedef struct master_list_s {
	char *name;
	module_list_t *module; 
} master_list_t;

#include "lst_net.h"
#include "lst_net_1000.h"
#include "lst_net_10_100.h"
#include "lst_net_arcnet.h"
#include "lst_net_other.h"
#include "lst_net_tokenring.h"
#include "lst_net_wan.h"
#include "lst_net_wlan.h"

static master_list_t list[] = {
	{ "net device", lst_net }, 
	{ "ethernet (1000)", lst_net_1000 },
	{ "ethernet (10/100)", lst_net_10_100 },
	{ "arcnet", lst_net_arcnet },
	{ "net: other", lst_net_other },
	{ "tokenring", lst_net_tokenring },
	{ "wan", lst_net_wan },
	{ "wlan", lst_net_wlan },
	{ NULL, 0 }
};

int main(void)
{
    struct pci_access *pacc;
    struct pci_dev *dev;
    int i, j, k;

    pacc = pci_alloc();		/* Get the pci_access structure */
    /* Set all options you want -- here we stick with the defaults */
    pci_init(pacc);		/* Initialize the PCI library */
    pci_scan_bus(pacc);		/* We want to get the list of devices */

    /* Iterate over all devices */
    for(dev=pacc->devices; dev; dev=dev->next) {
        /* Fillin header info we need */
        pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES);
        i=0;
	while (list[i].name != NULL ) {
            j=0;
            while (list[i].module[j].name != NULL) {
                k=0;
                while (list[i].module[j].id[k].vendor != 0) {
                    if ( (dev->vendor_id == list[i].module[j].id[k].vendor) &&
                        ( dev->device_id == list[i].module[j].id[k].device)) {
                        printf("match %s\n",list[i].module[j].name);
                        printf("vendor %x, device %x\n", 
                            list[i].module[j].id[k].vendor,
                            list[i].module[j].id[k].device);
                    }
                    k++;
                }
                j++;
            }
            i++;
        }
//      printf("vendor=%04x device=%04x\n",
//          dev->vendor_id, dev->device_id);
    }
    pci_cleanup(pacc);		/* Close everything */
    return(0);
}
