/*
 *
 */

/* net/dmfe.c v1.30 */
pci_ids_t dmfe_pci_ids[] = {
	{ 0x1282, 0x9132 },
	{ 0x1282, 0x9102 },
	{ 0x1282, 0x9100 },
	{ 0, }
};

/* net/irda/toshoboe, irdev */
pci_ids_t toshoboe_pci_ids[] = {
	{ 0x1179, 0x0701 },
	{ 0, }
};

/* net/ncr885e, really a scsi driver */
pci_ids_t ncr885_pci_ids[] = {
	{ 0x1000 , 0x0701 },
	{ 0, } 
};

/* net/sunhme, not sure if its really a PCI bus */
pci_ids_t sunhme_pci_ids[] = {
	{ 0x1011, 0x0025 },
	{ 0x108e, 0x1001 },
	{ 0, }
};

static module_list_t lst_net_other[] = {
	{ NULL, 0 }
};

