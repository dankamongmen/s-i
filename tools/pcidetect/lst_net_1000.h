/*
 *  Gigabit network card
 */

/* net/acenic.c */
pci_ids_t acenic_pci_ids[] = {
	{ 0x12ae, 0x0001 },
	{ 0x12ae, 0x0002 },
	{ 0x10b7, 0x0001 },
	{ 0x1385, 0x620a },
	{ 0x1385, 0x630a },
	
	{ 0x1011, 0x001a },
	{ 0x10a9, 0x0009 },
	{ 0, 0 }
};

/* net/hamachi.c v1.01; Packet Engines GNIC-II */
pci_ids_t hamachi_pci_ids[] = {
	{ 0x1318, 0x0911 },
	{ 0, 0 }
};

/* net/sk98lin.c */
pci_ids_t sk98lin_pci_ids[] = {
	{ 0x1148, 0x4300 },
	{ 0, 0 }
};

/* net/yellowfin.c v1.03a; Packet Engines GNIC */
pci_ids_t yellowfin_pci_ids[] = {
        { 0x1000, 0x0701 },
        { 0x1000, 0x0702 },
        { 0, 0 }
};

static module_list_t lst_net_1000[] = {
	/* 1000 Mbits */
	{ "acenic", acenic_pci_ids },
	{ "hamachi", hamachi_pci_ids },
	{ "sk98lin", sk98lin_pci_ids },
	{ "yellowfin", yellowfin_pci_ids },
	{ NULL, 0 }
};

