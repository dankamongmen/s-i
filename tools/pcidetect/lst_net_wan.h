/*
 *
 */

/* net/wan/sdladrv, */
pci_ids_t sdladrv_pci_ids[] = {
	{ 0x11b0, 0x0002 },
	{ 0, }
};

/* net/wan/lmc/lmc_main, need to check subvendor id as well */
pci_ids_t lmc_pci_ids[] = {
	{ 0x1011, 0x0009 },
	{ 0, }
};

static module_list_t lst_net_wan[] = {
	{ "sdladrv", sdladrv_pci_ids },
	{ "lmc", lmc_pci_ids },
	{ NULL, 0 }
};

