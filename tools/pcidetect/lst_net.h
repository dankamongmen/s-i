/*
 *
 */

/* net/defxx.c v1.05d */
pci_ids_t defxx_pci_ids[] = {
	{ 0x1011, 0x000f },
	{ 0, }
};

/* net/fc/iph5526 */
pci_ids_t iph5526_pci_ids[] = {
	{ 0x107e, 0x0004 },
	{ 0x107e, 0x0005 },
	{ 0, }
};

/* net/rcpi45 */
pci_ids_t rcpci_pci_ids[] = {
	{ 0x4916, 0x1960 },
	{ 0, }
};

/* net/rrunner */
pci_ids_t rrunner_pci_ids[] = {
	{ 0x120f, 0x0001 },
	{ 0, }
};

/* net/skfp, v2.06 */
pci_ids_t skfp_pci_ids[] = {
	{ 0x1148, 0x4000 },
	{ 0, }
};

static module_list_t lst_net[] = {
	{ "defxx", defxx_pci_ids },
	{ "iph5526", iph5526_pci_ids },
	{ "rcpci", rcpci_pci_ids },
	{ "rrunner", rrunner_pci_ids },
	{ "skfp", skfp_pci_ids },	
	{ NULL, 0 }
};

