/*
 *
 */

/* net/tokenring/abyss.c, v1.01 */
pci_ids_t abyss_pci_ids[] = {
	{ 0x10b6, 0x0002 },
	{ 0, }
};

/* net/tokenring/lanstreamer, v0.3.1 */
pci_ids_t lanstreamer_pci_ids[] = {
	{ 0x1014 , 0x0018 },
	{ 0, }
};

/* net/tokenring/olympic, v0.5.0 */
pci_ids_t olympic_pci_ids[] = {	
	{ 0x1014, 0x00e3 },
	{ 0, }
};

/* net/tokenring/tmspci */
pci_ids_t tmspci_pci_ids[] = {
	{ 0x0e11, 0x0508 },
	{ 0x1148, 0x4200 },
	{ 0x10da, 0x508 },
	{ 0x10b7, 0x3390 },
	{ 0, }
};

static module_list_t lst_net_tokenring[] = {
        { "abyss", abyss_pci_ids },
	{ "lanstreamer", lanstreamer_pci_ids },
	{ "olympic", olympic_pci_ids },
	{ "tmspci", tmspci_pci_ids },
	{ NULL, 0 }
};

