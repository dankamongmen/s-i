/*
 * Copyright (C) 2001 by Glenn McGrath <bug1@debian.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <pci/pci.h>

typedef struct pci_ids_s {
	unsigned int vendor;
	unsigned int device;
} pci_ids_t;

typedef struct module_list_s {
	char *name;
	pci_ids_t *id;	
} module_list_t;

#include "pci_ids.h"

int main(void)
{
    struct pci_access *pacc;
    struct pci_dev *dev;
    int i;
	int j;

	/* Get the pci_access structure */
    pacc = pci_alloc();

    /* Set all options you want -- here we stick with the defaults */
	/* Initialize the PCI library */
    pci_init(pacc);

	/* We want to get the list of devices */
	pci_scan_bus(pacc);

	/* Iterate over all devices */
	for (dev = pacc->devices; dev; dev = dev->next) {
		/* Fillin header info we need */
		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES);
		i = 0;
		while (module_list[i].name != NULL ) {
			j = 0;
			while (module_list[i].id[j].vendor != 0) {
				if ((dev->vendor_id == module_list[i].id[j].vendor) &&
						(dev->device_id == module_list[i].id[j].device)) {
					printf("%s.o\n", module_list[i].name);
//					printf("vendor %x, device %x\n", 
//						module_list[i].id[j].vendor,
//						module_list[i].id[j].device);
                }
                j++;
            }
            i++;
        }
    }
    pci_cleanup(pacc);		/* Close everything */
    return(0);
}
