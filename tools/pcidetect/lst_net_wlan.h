/*
 *
 */

/* net/aironet4500_card.c */
pci_ids_t aironet_pci_ids[] = {
	{ 0x14b9, 0x0001 },
	{ 0x14b9, 0x4500 },
	{ 0x14b9, 0x4800 },
	{ 0, }
};

static module_list_t lst_net_wlan[] = {
	{ "aironet", aironet_pci_ids },
	{ NULL, 0 }
};

