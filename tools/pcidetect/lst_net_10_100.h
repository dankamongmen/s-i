/*
 *
 */

/* net/3x59x.c, vLK1.1.9 */
pci_ids_t com3_pci_ids[] = {
	{ 0x10b7, 0x5900 },
	{ 0x10b7, 0x5920 },
	{ 0x10b7, 0x5970 },
        { 0x10b7, 0x5950 },
        { 0x10b7, 0x5951 },

        { 0x10b7, 0x5952 },
        { 0x10b7, 0x9000 },
        { 0x10b7, 0x9001 },
        { 0x10b7, 0x9004 },
        { 0x10b7, 0x9005 },

        { 0x10b7, 0x9006 },
        { 0x10b7, 0x900a },
        { 0x10b7, 0x9050 },
        { 0x10b7, 0x9051 },
        { 0x10b7, 0x9055 },

        { 0x10b7, 0x9058 },
        { 0x10b7, 0x905a },
	{ 0x10b7, 0x9200 },
	{ 0x10b7, 0x9800 },
	{ 0x10b7, 0x9805 },

	{ 0x10b7, 0x7646 },
	{ 0x10b7, 0x5055 },
	{ 0x10b7, 0x6055 },
	{ 0x10b7, 0x5b57 },
	{ 0x10b7, 0x5057 },

	{ 0x10b7, 0x5157 },
	{ 0x10b7, 0x5257 },
	{ 0x10b7, 0x6560 },
	{ 0x10b7, 0x6562 },
	{ 0x10b7, 0x6564 },

	{ 0x10b7, 0x4500 },
	{ 0, }
};

/* net/8139too.c, v0.9.11 */
pci_ids_t too8139_pci_ids[] = {
	{ 0x10ec, 0x8139 },
	{ 0x10ec, 0x8138 },
	{ 0x1113, 0x1211 },
	{ 0x1500, 0x1360 },
	{ 0x4033, 0x1360 },

	{ 0, }
};

/* net/de4x5 */
pci_ids_t de4x5_pci_ids[] = {
	{ 0x00ff, 0xff00 },
	{ 0x1011, 0x0200 },
	{ 0x1011, 0x1400 },
	{ 0x1011, 0x0900 },
	{ 0x1011, 0x1900 },
	{ 0, }
};

/* net/eepro100.c v1.33 */
pci_ids_t eepro100_pci_ids[] = {
	{ 0x8086, 0x1229 },
	{ 0x8086, 0x1209 },
	{ 0x8086, 0x1029 },
	{ 0x8086, 0x1030 },
	{ 0x8086, 0x2449 },
	
	{ 0, }
};

/* net/epic100.c v1.1.5 */
pci_ids_t epic100_pci_ids[] = {
	{ 0x10b8, 0x0005 },
	{ 0x10b8, 0x0006 },	
	{ 0, }
};

/* net/gmac.c, v1.2k4, im not sure if this will work */
pci_ids_t gmac_pci_ids[] = {
	{ 0x106b, 0x0021 },
	{ 0, }
};

/* net/hp100.c, v1.57 */
pci_ids_t hp100_pci_ids[] = {
	{ 0x103c, 0x1030 },
	{ 0x103c, 0x1031 },
	{ 0x11f6, 0x101a },
	{ 0x101a, 0x0005 },
	{ 0, }
};

/* net/ioc3-eth.c */
pci_ids_t ioc3_pci_ids[] = {
        { 0x10a9, 0x0003 },
        { 0, }
};

/* net/lance.c v1.15ac */ 	/* pcnet also seems to cover this, weird */
pci_ids_t lance_pci_ids[] = {
	{ 0x1022, 0x2000 }, 
	{ 0, }
};

/* net/natsemi.c, v1.05 */
pci_ids_t natsemi_pci_ids[] = {
	{ 0x100b, 0x0020 },
	{ 0, }
};

/* net/ne.c, v1.10 */
pci_ids_t ne_pci_ids[] = {
	{ 0x10ec, 0x8029 },
	{ 0x1050, 0x0940 },
	{ 0x11f6, 0x1401 },
	{ 0x8e2e, 0x3000 },
	{ 0x4a14, 0x5000 },

	{ 0x1106, 0x0926 },
	{ 0x10bd, 0x0e34 },
	{ 0, }
};

/* net/ne2k-pci.c, vpre-1.00e */
pci_ids_t ne2k_pci_ids[] = {
        { 0x10ec, 0x8029 },
        { 0x1050, 0x0940 },
        { 0x11f6, 0x1401 },
        { 0x8e2e, 0x3000 },
        { 0x4a14, 0x5000 },
      
	{ 0x1106, 0x0926 },
        { 0x10bd, 0x0e34 },
        { 0x1050, 0x5a5a },
	{ 0x12c3, 0x0058 },
        { 0x12c3, 0x5598 },

        { 0, }
};

/* net/pcnet32.c v1.25kf*/
pci_ids_t pcnet32_pci_ids[] = {
	{ 0x1022, 0x2000},
	{ 0x1022, 0x2001},
	{ 0, }
};

/* net/rtl8129.c v1.07*/
pci_ids_t rtl8129_pci_ids[] = {
        { 0x10ec, 0x8129 },
        { 0, }
};

/* net/sis900.c v 1.07.04 */
pci_ids_t sis900_pci_ids[] = {
	{ 0x1039, 0x0900 },
	{ 0x1039, 0x7016 },
	{ 0, }
};

/* net/sundance.c v1.01 */
pci_ids_t sundance_pci_ids[] = {
	{ 0x1186, 0x1002 },
	{ 0x13f0, 0x0201 },
	{ 0, }
};

/* net/tlan.c v1.12 */
pci_ids_t tlan_pci_ids[] = {
	{ 0x0e11, 0xae34 },
	{ 0x0e11, 0xae32 },
	{ 0x0e11, 0xae35 },
	{ 0x0e11, 0xf130 },
	{ 0x0e11, 0xf150 },
	
	{ 0x0e11, 0xae43 },
  	{ 0x0e11, 0xae40 },
	{ 0x0e11, 0xb011 },
	{ 0x0e11, 0x0013 },
	{ 0x0e11, 0x0012 },
	
	{ 0x0e11, 0x0014 },
	{ 0x0e11, 0xb012 },
	{ 0x0e11, 0xb030 },
	{ 0, }
};

/* net/tulip_core.c v0.9.10 */
pci_ids_t tulip_pci_ids[] = {
	{ 0x1011, 0x0002 },
	{ 0x1011, 0x0014 },
	{ 0x1011, 0x0009 },
	{ 0x1011, 0x0019 },
	{ 0x11ad, 0x0002 },
	
	{ 0x10d9, 0x0512 },
	{ 0x10d9, 0x0531 },
	{ 0x125b, 0x1400 },
	{ 0x11ad, 0xc115 },
	{ 0x1317, 0x0981 },
	
	{ 0x1317, 0x0985 },
	{ 0x1317, 0x1985 },
	{ 0x13d1, 0xab02 },
	{ 0x13d1, 0xab03 },
	{ 0x104a, 0x0981 },
	
	{ 0x104a, 0x2774 },
	{ 0x11f6, 0x9881 },
	{ 0x8086, 0x0039 },
	{ 0x1282, 0x9100 },
	{ 0x1282, 0x9102 },
	
	{ 0x1113, 0x1217 },
	{ 0, }
};

/* net/via-rhine.c vLK1.1.6 */
pci_ids_t via_rhine_pci_ids[] = {
	{ 0x1106, 0x6100 },
	{ 0x1106, 0x3065 },
	{ 0x1106, 0x3043 },
	{ 0, }
};

/* net/winbond-840.c v1.01 */
pci_ids_t winbond_pci_ids[] = {
	{ 0x1050, 0x0840 },
	{ 0x11f6, 0x2011 },
	{ 0, }
};

static module_list_t lst_net_10_100[] = {
        { "3c59x", com3_pci_ids },
	{ "8139too", too8139_pci_ids },
	{ "defxx", defxx_pci_ids },
//	{ "dgrs", dgrs_pci_ids },
//	{ "dmfe", dmfe_pci_ids },
	{ "eepro100", eepro100_pci_ids },
	{ "epic100", epic100_pci_ids },         /* SMC EtherPower II */
	{ "hp100", hp100_pci_ids },
	{ "ioc3-eth", ioc3_pci_ids },
	{ "lance", lance_pci_ids },
	{ "natsemi", natsemi_pci_ids }, 	
        { "ne2k-pci", ne2k_pci_ids },
	{ "pcnet32", pcnet32_pci_ids },         /* AMD PCnet32 */
	{ "rtl8129", rtl8129_pci_ids },
	{ "sis900", sis900_pci_ids },
//	{ "starfire", starfire_pci_ids },       /* Adaptec Starfire */
	{ "sundance", sundance_pci_ids },
	{ "tlan", tlan_pci_ids },               /* TI ThunderLAN */
	{ "tulip", tulip_pci_ids },             /* dc21x4x family */
	{ "via-rhine", via_rhine_pci_ids },
	{ "winbond", winbond_pci_ids },
	{ NULL, 0 }
};

