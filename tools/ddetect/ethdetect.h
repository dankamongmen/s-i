isa_dev_id_t _3c509_id[] = {
{ "TCB5091" }, /* 3Com : 3c590B Combo ISA-PnP */
{ "TCM5090" }, /* 3Com : EtherLink III ISA (3C509-TP) */
{ "TCM5091" }, /* 3Com : EtherLink III ISA (3C509) */
{ "TCM5092" }, /* 3Com : EtherLink III EISA (3C579-TP) */
{ "TCM5093" }, /* 3Com : EtherLink III EISA (3C579) */
{ "TCM5094" }, /* 3Com : EtherLink III ISA (3C509-Combo) */
{ "TCM5095" }, /* 3Com : EtherLink III ISA (3C509-TPO) */
{ "TCM5098" }, /* 3Com : 509B Ethernet Card */
{0} };


#if 0
isa_dev_id_t _ignore_id[] = {
{ "ADV55AA" }, /* AMD : PCNET Family Ethernet/ISA+ */
{ "SKD0100" }, /* SysKonnect : SK-NET FDDI-FE EISA 32-Bit FDDI LAN */
{ "SKD0200" }, /* SysKonnect : SK-NET G32+ EISA 32-Bit Ethernet/IEEE802.3 */
{ "SKD8000" }, /* SysKonnect : SK-NET Flash Ethernet */
{ "SKDD300" }, /* SysKonnect : SK-NET TR4/16+ Token-Ring */
{ "SKDE000" }, /* SysKonnect : SK-NET G16 LAN */
{ "SKDF000" }, /* SysKonnect : SK-NET FDDI-FI LAN */
{ "SMC8000" }, /* SMC : SMC8232 Network Adapter */
{ "SMC8003" }, /* SMC : EtherCard PLUS Elite */
{ "SMC8010" }, /* SMC : EtherCard Elite Ultra 32 */
{ "SMC8013" }, /* SMC : EtherCard PLUS Elite16 */
{ "SMC8115" }, /* SMC : TokenCard Elite Family LAN Adapters */
{ "SMC8216" }, /* SMC : EtherCard Elite16 ULTRA */
{ "SMC8416" }, /* SMC : EtherEZ (8416) */
{ "SMC9000" }, /* SMC : 9000 Ethernet Adapter */
{ "SMCA010" }, /* SMC : Ether 10/100 Fast Ethernet EISA Network Adapter */
{0} };


isa_dev_id_t _unknown_id[] = {
{ "CSI2201" }, /* Cabletron : DNI series E2200 */
{ "CSI2202" }, /* Cabletron : DNI series E2200 */
{ "CSI2203" }, /* Cabletron : DNI series E2200 */
{0} };
#endif

isa_dev_id_t _ne_id[] = {
{ "@@@9519" }, /* unknown : NE2000 plug&play ethernet card
 */
{ "AXE2201" }, /* Xerox Corporation : SN2000 */
{ "BTU3840" }, /* Boca Research : Lancard 2000 plus combo */
{ "DLK2201" }, /* D-Link : DE-220 NE2000 compatable Network Card */
{ "ICU02A0" }, /* D-Link : DE-100 */
{ "ICU02B0" }, /* D-Link : DE-200 */
{ "ICU02C0" }, /* Eagle/Novell : NE1000 (810-160-00X) */
{ "ICU02C1" }, /* Eagle/Novell : NE1000 (950-054401) */
{ "ICU02D0" }, /* Eagle/Novell : NE1500T */
{ "ICU02E0" }, /* Eagle/Novell : NE2100 */
{ "ICU19D0" }, /* Unknown : NE 2000plus */
{ "JQE9008" }, /* CNet : CN200E plus */
{ "LNE2000" }, /* Linksys : Ether16 LAN Card */
{ "PNP80D4" }, /* Novell/Anthem : NE2000 */
{ "PNP80D6" }, /* Unknown : NE2000 Compatible */
{ "PNP80D9" }, /* Unknown : NE2000 Plus */
{ "PNP820A" }, /* Zenith Data Systems : NE2000-Compatible */
{ "PNP82C0" }, /* Eagle Technology : NE200T */
{ "RTL8019" }, /* Realtek : RTL8019 Ethernet NIC */
{ "RTL8029" }, /* Realtek : RTL8029 Ethernet NIC */
{ "SZV4953" }, /* OvisLink : NE-2000 Compatible Ethernet NIC */
{0} };



isa_dev_id_t _ne2000_id[] = {
{ "SMC1660" }, /* SMC : EZ Card (1660) */
{0} };



isa_module_list_t isa_modules[] = {
{ "3c509", _3c509_id }, 
/*{ "ignore", _ignore_id }, */
{ "ne", _ne_id }, 
{ "ne2000", _ne2000_id }, 
{ NULL, NULL} };



pci_ids_t _3c59x_id[] = {
{ 0x10b75900 }, /* 3Com Corporation : 3c590 10BaseT [Vortex] */
{ 0x10b75950 }, /* 3Com Corporation : 3c595 100BaseTX [Vortex] */
{ 0x10b75951 }, /* 3Com Corporation : 3c595 100BaseT4 [Vortex] */
{ 0x10b75952 }, /* 3Com Corporation : 3c595 100Base-MII [Vortex] */
{ 0x10b77646 }, /* 3Com Corporation : 3cSOHO100-TX Hurricane */
{ 0x10b79000 }, /* 3Com Corporation : 3c900 10BaseT [Boomerang] */
{ 0x10b79001 }, /* 3Com Corporation : 3c900 Combo [Boomerang] */
{ 0x10b79004 }, /* 3Com Corporation : 3c900B-TPO [Etherlink XL TPO] */
{ 0x10b79005 }, /* 3Com Corporation : 3c900B-Combo [Etherlink XL Combo] */
{ 0x10b79006 }, /* 3Com Corporation : 3c900B-TPC [Etherlink XL TPC] */
{ 0x10b7900a }, /* 3Com Corporation : 3c900B-FL [Etherlink XL FL] */
{ 0x10b79050 }, /* 3Com Corporation : 3c905 100BaseTX [Boomerang] */
{ 0x10b79051 }, /* 3Com Corporation : 3c905 100BaseT4 */
{ 0x10b79055 }, /* 3Com Corporation : 3c905B 100BaseTX [Cyclone] */
{ 0x10b79058 }, /* 3Com Corporation : 3c905B-Combo [Deluxe Etherlink XL 10/100] */
{ 0x10b7905a }, /* 3Com Corporation : 3c905B-FX [Fast Etherlink XL FX 10/100] */
{ 0x10b79200 }, /* 3Com Corporation : 3c905C-TX [Fast Etherlink] */
{ 0x10b79800 }, /* 3Com Corporation : 3c980-TX [Fast Etherlink XL Server Adapter] */
{0} };



pci_ids_t _acenic_id[] = {
{ 0x12ae0001 }, /* Alteon Networks Inc. : AceNIC Gigabit Ethernet */
{ 0x1385620a }, /* Netgear : GA620 */
{0} };



pci_ids_t _dgrs_id[] = {
{ 0x114f0003 }, /* Digi International : RightSwitch SE-6 */
{0} };



pci_ids_t _dmfe_id[] = {
{ 0x12829102 }, /* Davicom Semiconductor, Inc. : Ethernet 100/10 MBit */
{0} };



pci_ids_t _eepro100_id[] = {
{ 0x10c31100 }, /* Samsung Semiconductors, Inc. : Smartether100 SC1100 LAN Adapter (i82557B) */
{ 0x12592560 }, /* Allied Telesyn International : AT-2560 Fast Ethernet Adapter (i82557B) */
{ 0x12660001 }, /* Microdyne Corporation : NE10/100 Adapter (i82557B) */
{ 0x80861227 }, /* Intel Corporation : 82865 [Ether Express Pro 100] */
{ 0x80861228 }, /* Intel Corporation : 82556 [Ether Express Pro 100 Smart] */
{ 0x80861229 }, /* Intel Corporation : 82557 [Ethernet Pro 100] */
{ 0x80865200 }, /* Intel Corporation : EtherExpress PRO/100 */
{ 0x80865201 }, /* Intel Corporation : EtherExpress PRO/100 */
{0} };



pci_ids_t _epic100_id[] = {
{ 0x10b80005 }, /* Standard Microsystems Corp [SMC] : 83C170QF */
{ 0x10b80006 }, /* Standard Microsystems Corp [SMC] : LANEPIC */
{0} };



pci_ids_t _hp100_id[] = {
{ 0x103c1030 }, /* Hewlett-Packard Company : J2585A */
{ 0x103c1031 }, /* Hewlett-Packard Company : J2585B */
{ 0x11f60112 }, /* Compex : ENet100VG4 */
{0} };



pci_ids_t _ibmtr_id[] = {
{ 0x108d0002 }, /* Olicom : 16/4 Token Ring */
{ 0x108d0004 }, /* Olicom : RapidFire 3139 Token-Ring 16/4 PCI Adapter */
{ 0x108d0005 }, /* Olicom : GoCard 3250 Token-Ring 16/4 CardBus PC Card */
{ 0x108d0007 }, /* Olicom : RapidFire 3141 Token-Ring 16/4 PCI Fiber Adapter */
{0} };






pci_ids_t _ne2k_pci_id[] = {
{ 0x10500000 }, /* Winbond Electronics Corp : NE2000 */
{ 0x10500940 }, /* Winbond Electronics Corp : W89C940 */
{ 0x10bd0e34 }, /* Surecom Technology : NE-34 */
{ 0x10ec8029 }, /* Realtek Semiconductor Co., Ltd. : RTL-8029(AS) */
{ 0x11060926 }, /* VIA Technologies, Inc. : VT82C926 [Amazon] */
{ 0x11f61401 }, /* Compex : ReadyLink 2000 */
{ 0x11f62201 }, /* Compex : ReadyLink 100TX (Winbond W89C840) */
{ 0x4a145000 }, /* NetVin : NV5000SC */
{ 0x8e2e3000 }, /* KTI : ET32P2 */
{0} };



pci_ids_t _old_tulip_id[] = {
{ 0x10110009 }, /* Digital Equipment Corporation : DECchip 21140 [FasterNet] */
{0} };



pci_ids_t _rtl8139_id[] = {
{ 0x10ec8129 }, /* Realtek Semiconductor Co., Ltd. : RTL-8129 */
{ 0x10ec8138 }, /* Realtek Semiconductor Co., Ltd. : RT8139 (B/C) Cardbus Fast Ethernet Adapter */
{ 0x10ec8139 }, /* Realtek Semiconductor Co., Ltd. : RTL-8139 */
{ 0x11131211 }, /* Accton Technology Corporation : SMC2-1211TX */
{0} };



pci_ids_t _sis900_id[] = {
{ 0x10390900 }, /* Silicon Integrated Systems [SiS] : SiS900 10/100 Ethernet */
{0} };



pci_ids_t _sktr_id[] = {
{ 0x0e110508 }, /* Compaq Computer Corporation : Netelligent 4/16 Token Ring */
{ 0x11484200 }, /* Syskonnect (Schneider & Koch) : Token ring adaptor */
{0} };



pci_ids_t _tlan_id[] = {
{ 0x0e11ae32 }, /* Compaq Computer Corporation : Netelligent 10/100 */
{ 0x0e11ae34 }, /* Compaq Computer Corporation : Netelligent 10 */
{ 0x0e11ae35 }, /* Compaq Computer Corporation : Integrated NetFlex-3/P */
{ 0x0e11ae40 }, /* Compaq Computer Corporation : Netelligent 10/100 Dual */
{ 0x0e11ae43 }, /* Compaq Computer Corporation : ProLiant Integrated Netelligent 10/100 */
{ 0x0e11b011 }, /* Compaq Computer Corporation : Integrated Netelligent 10/100 */
{ 0x0e11b012 }, /* Compaq Computer Corporation : Netelligent 10 T/2 */
{ 0x0e11b030 }, /* Compaq Computer Corporation : Netelligent WS 5100 */
{ 0x0e11f130 }, /* Compaq Computer Corporation : NetFlex-3/P ThunderLAN 1.0 */
{ 0x0e11f150 }, /* Compaq Computer Corporation : NetFlex-3/P ThunderLAN 2.3 */
{ 0x108d0012 }, /* Olicom : OC-2325 */
{ 0x108d0013 }, /* Olicom : OC-2183/2185 */
{ 0x108d0014 }, /* Olicom : OC-2326 */
{ 0x108d0019 }, /* Olicom : OC-2327/2250 10/100 Ethernet Adapter */
{0} };



pci_ids_t _tulip_id[] = {
{ 0x10110002 }, /* Digital Equipment Corporation : DECchip 21040 [Tulip] */
{ 0x10110014 }, /* Digital Equipment Corporation : DECchip 21041 [Tulip Pass 3] */
{ 0x10110019 }, /* Digital Equipment Corporation : DECchip 21142/43 */
{ 0x10d90512 }, /* Macronix, Inc. [MXIC] : MX98713 */
{ 0x10d90531 }, /* Macronix, Inc. [MXIC] : MX987x5 */
{ 0x10d98625 }, /* Macronix, Inc. [MXIC] : MX86250 */
{ 0x11860100 }, /* D-Link System Inc : DC21041 */
{ 0x11ad0002 }, /* Lite-On Communications Inc : LNE100TX */
{ 0x11adc115 }, /* Lite-On Communications Inc : LNE100TX Fast Ethernet Adapter */
{ 0x11f69881 }, /* Compex : RL100TX */
{ 0x80860039 }, /* Intel Corporation : 21145 */
{0} };

#if 0
pci_ids_t _ignore_id[] = {
{ 0x10500840 }, /* Winbond Electronics Corp : W89C840 */
{0} };

pci_ids_t _unknown_id[] = {
{ 0x103c1064 }, /* Hewlett-Packard Company : 79C970 PCnet Ethernet Controller */
{ 0x12661910 }, /* Microdyne Corporation : NE2000Plus (RT8029) Ethernet Adapter */
{ 0x13180911 }, /* Packet Engines Inc. : PCI Ethernet Adapter */
{0} };
#endif


pci_ids_t _via_rhine_id[] = {
{ 0x11063043 }, /* VIA Technologies, Inc. : VT86C100A [Rhine 10/100] */
{ 0x11066100 }, /* VIA Technologies, Inc. : VT85C100A [Rhine II] */
{0} };



pci_module_list_t pci_modules[] = {
{ "3c59x", _3c59x_id }, 
{ "acenic", _acenic_id }, 
{ "dgrs", _dgrs_id }, 
{ "dmfe", _dmfe_id }, 
{ "eepro100", _eepro100_id }, 
{ "epic100", _epic100_id }, 
{ "hp100", _hp100_id }, 
{ "ibmtr", _ibmtr_id }, 
/* { "ignore", _ignore_id }, */
{ "ne2k-pci", _ne2k_pci_id }, 
{ "old_tulip", _old_tulip_id }, 
{ "rtl8139", _rtl8139_id }, 
{ "sis900", _sis900_id }, 
{ "sktr", _sktr_id }, 
{ "tlan", _tlan_id }, 
{ "tulip", _tulip_id }, 
/* { "unknown", _unknown_id },  */
{ NULL, NULL} };
