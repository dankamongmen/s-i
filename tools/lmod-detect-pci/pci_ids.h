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

pci_ids_t _3c59x_ids[] = {
	{ 4279, 17664 },
	{ 4279, 20565 },
	{ 4279, 20567 },
	{ 4279, 20823 },
	{ 4279, 21079 },
	{ 4279, 22784 },
	{ 4279, 22816 },
	{ 4279, 22864 },
	{ 4279, 22865 },
	{ 4279, 22866 },
	{ 4279, 22896 },
	{ 4279, 23383 },
	{ 4279, 24661 },
	{ 4279, 24662 },
	{ 4279, 25952 },
	{ 4279, 25954 },
	{ 4279, 25956 },
	{ 4279, 30278 },
	{ 4279, 36864 },
	{ 4279, 36865 },
	{ 4279, 36868 },
	{ 4279, 36869 },
	{ 4279, 36870 },
	{ 4279, 36874 },
	{ 4279, 36944 },
	{ 4279, 36945 },
	{ 4279, 36949 },
	{ 4279, 36952 },
	{ 4279, 36954 },
	{ 4279, 37376 },
	{ 4279, 38912 },
	{ 4279, 38917 },
	{ 0, 0}
};

#if 0
"53c7,8xx",256,4096,1
"53c7,8xx",256,4096,2
"53c7,8xx",256,4096,3
"53c7,8xx",256,4096,4
#endif

pci_ids_t _8139too_ids[] = {
	{ 4332, 33065 },
	{ 4332, 33080 },
	{ 4332, 33081 },
	{ 4371, 4625 },
	{ 4486, 4864 },
	{ 5376, 4960 },
	{ 16435,4960 },
	{ 0, 0 }
};

pci_ids_t abyss_ids[] = {
	{ 4278, 2 },
	{ 0, 0 }
};

pci_ids_t acenic_ids[] = {
	{ 4113, 26 },
	{ 4265, 9 },
	{ 4279, 1 },
	{ 4782, 1 },
	{ 4782, 2 },
	{ 4997, 25098 },
	{ 4997, 25354 },
	{ 0, 0 }
};

pci_ids_t aec62xx_ids[] = {
	{ 4497, 5 },
	{ 4497, 6 },
	{ 4497, 7 },
	{ 0, 0 }
};

pci_ids_t agpart_ids[] = {
	{ 4130, 28678 },
	{ 4153, 1328 },
	{ 4153, 1344 },
	{ 4153, 1568 },
	{ 4153, 1584 },
	{ 4281, 5441 },
	{ 4281, 5665 },
	{ 4281, 5681 },
	{ 4281, 5682 },
	{ 4281, 5697 },
	{ 4281, 5703 },
	{ 4281, 5713 },
	{ 4358, 773 },
	{ 4358, 913 },
	{ 4358, 1281 },
	{ 4358, 1431 },
	{ 4358, 1432 },
	{ 4358, 1681 },
	{ 4454, 8 },
	{ 4454, 9 },
	{ 32902, 4400 },
	{ 32902, 4402 },
	{ 32902, 6689 },
	{ 32902, 9520 },
	{ 32902, 28960 },
	{ 32902, 28961 },
	{ 32902, 28962 },
	{ 32902, 28963 },
	{ 32902, 28964 },
	{ 32902, 28965 },
	{ 32902, 29056 },
	{ 32902, 29072 },
	{ 32902, 29088 },
	{ 0, 0 }
};

pci_ids_t aic5800_ids[] = {
	{ 36868, 22528 },
	{ 0 , 0 }
};

pci_ids_t aic7xxx_old_ids[] = {
	{ 36868,4216 },
	{ 36868,8568 },
	{ 36868,14432 },
	{ 36868,20600 },
	{ 36868,21880 },
	{ 36868,24632 },
	{ 36868,24693 },
	{ 36868,24696 },
	{ 36868,24952 },
	{ 36868,28792 },
	{ 36868,29048 },
	{ 36868,29304 },
	{ 36868,29560 },
	{ 36868,29816 },
	{ 36868,30869 },
	{ 36868,32888 },
	{ 36868,33144 },
	{ 36868,33400 },
	{ 36868,33656 },
	{ 36868,33912 },
	{ 36868,34168 },
	{ 36868,34424 },
	{ 36868,34680 },
	{ 36868, 34936 },
	{ 36869, 16 },
	{ 36869, 17 },
	{ 36869, 19 },
	{ 36869, 31 },
	{ 36869, 80 },
	{ 36869, 81 },
	{ 36869, 95 },
	{ 36869, 128 },
	{ 36869, 129 },
	{ 36869, 131 },
	{ 36869, 143 },
	{ 36869, 192 },
	{ 36869, 193 },
	{ 36869, 195 },
	{ 36869, 207 },
	{ 0, 0 }
};

pci_ids_t alim15x3_ids[] = {
	{ 4281, 5427 },
	{ 0, 0 }
};

#if 0
pci_ids_t acenic_ids[] = {
AM53C974,256,4130,8224
};
#endif

pci_ids_t ambassador_ids[] = {
	{ 4278,4097 },
	{ 4278,4098 },
	{ 0, 0 }
};

pci_ids_t applicom_ids[] = {
	{ 5001,1 }, 
	{ 5001,2 },
	{ 5001,3 },
	{ 0, 0 }
};

pci_ids_t aty128fb_ids[] = {
	{ 4098, 19525 },
	{ 4098, 19526 },
	{ 4098, 20550 },
	{ 4098, 20562 },
	{ 4098, 21061 },
	{ 4098, 21062 },
	{ 4098, 21067 },
	{ 4098, 21068 },
	{ 0, 0 }
};

pci_ids_t avm_ids[] = {
	{ 4676, 2560 },
	{ 0, 0 }
};

pci_ids_t b1pci_ids[] = {
	{ 4676,1792 },
	{ 0, 0 }
};

pci_ids_t bkm_a4t_ids[] = {
	{ 2161,65444 },
	{ 4574,24864 },
	{ 0, 0 }
};

pci_ids_t bkm_a8_ids[] = {
	{ 2161,65448 },
	{ 4277,36944 },
	{ 0, 0 }
};

pci_ids_t bttv_ids[] = {
	{ 4254, 848 },
	{ 4254, 849 },
	{ 4254, 878 },
	{ 4254, 879 },
	{ 32902,4663 },
	{ 0, 0 }
};

pci_ids_t buz_ids[] = {
	{ 4574, 24663 },
	{ 0, 0 }
};

pci_ids_t c4_ids[] = {
	{ 4113, 4197 },
	{ 0, 0 }
};

pci_ids_t cciss_ids[] = {
	{ 3601, 45152 },
	{ 3601, 45432 },
	{ 0, 0 }
};

pci_ids_t chipsfb_ids[] = {
	{ 4140, 224 },
	{ 0, 0 }
};

pci_ids_t clgenfb_ids[] = {
	{ 4115, 160 },
	{ 4115, 164 },
	{ 4115, 168 },
	{ 4115, 172 },
	{ 4115, 184 },
	{ 4115, 188 },
	{ 4115, 208 },
	{ 4115, 212 },
	{ 4115, 214 },
	{ 0, 0 }
};

pci_ids_t cmpci_ids[] = {
	{ 5110, 256 },
	{ 5110, 257 },
	{ 5110, 273 },
	{ 5110, 274 },
	{ 0, 0 }
};

pci_ids_t cpqarray_ids[] = {
	{ 3601, 44560 },
	{ 4096, 16 },
	{ 0, 0 }
};

pci_ids_t cs4281_ids[] = {
	{ 4115, 24581 },
	{ 0, 0 }
};

pci_ids_t cs461x_ids[] = {
	{ 4115, 24577 },
	{ 4115, 24579 },
	{ 4115, 24581 },
	{ 4115, 24577 },
	{ 4115, 24579 },
	{ 4115, 24580 },
	{ 0, 0 }
};

pci_ids_t cyber2000fb_ids[] = {
	{ 4330, 8192 },
	{ 4330, 8208 },
	{ 4330, 20480 },
	{ 0, 0 }
};

pci_ids_t cyclades_ids[] = {
	{ 4622, 256 },
	{ 4622, 257 },
	{ 4622, 258 },
	{ 4622, 259 },
	{ 4622, 260 },
	{ 4622, 261 },
	{ 4622, 512 },
	{ 4622, 513 },
	{ 0, 0 }
};

pci_ids_t defxx_ids[] = {
	{ 4113, 15 },
	{ 0, 0 }
};

pci_ids_t dgrs_ids[] = {
	{ 4431, 3 },
	{ 0, 0 }
};

pci_ids_t diva_ids[] = {
	{ 4403, 57346 },
	{ 4403, 57348 },
	{ 4403, 57349 },
	{ 0, 0 }
};

pci_ids_t dmfe_ids[] = {
	{ 4738, 36873 },
	{ 4738, 37120 },
	{ 4738, 37122 },
	{ 4738, 37170 },
	{ 0, 0 }
};

pci_ids_t dmx3191d_ids[] = {
	{ 4938, 1 },
	{ 0, 0 }
};

pci_ids_t dscc4_ids[] = {
	{ 4362, 8450 },
	{ 0, 0 }
};

pci_ids_t eepro100_ids[] = {
	{ 32902, 4137 },
	{ 32902, 4144 },
	{ 32902, 4617 },
	{ 32902, 4649 },
	{ 32902, 9289 },
	{ 0, 0 }
};

pci_ids_t elsa_ids[] = {
	{ 4168, 4096 },
	{ 4168, 12288 },
	{ 0, 0 }
};

pci_ids_t emu10k1_ids[] = {
	{ 4354, 2 },
	{ 0, 0 }
};

pci_ids_t eni_ids[] = {
	{ 4378, 0 },
	{ 4378, 2 },
	{ 0, 0 }
};

pci_ids_t epca_ids[] = {
	{ 4431, 4 },
	{ 4431, 5 },
	{ 4431, 6 },
	{ 4431, 9 },
	{ 0, 0 }
};

pci_ids_t epic100_ids[] = {
	{ 4280, 5 },
	{ 4280, 6 },
	{ 0, 0 }
};

pci_ids_t es1370_ids[] = {
	{ 4724, 20480 },
	{ 0, 0 }
};

pci_ids_t es1371_ids[] = {
	{ 4354, 35128 },
	{ 4724, 4977 },
	{ 4724, 22656 },
	{ 0, 0 }
};

pci_ids_t esssolo1_ids[] = {
	{ 4701, 6505 },
	{ 0, 0 }
};

pci_ids_t fdomain_ids[] = {
	{ 4150, 0 },
	{ 0, 0 }
};

pci_ids_t fealnx_ids[] = {
	{ 5398, 2048 },
	{ 5398, 2051 },
	{ 5398, 2193 },
	{ 0, 0 }
};

pci_ids_t firestream_ids[] = {
	{ 4510, 1 },
	{ 4510, 3 },
	{ 0, 0 }
};

pci_ids_t fore_200e_ids[] = {
	{ 4391, 768 },
	{ 0, 0 }
};

pci_ids_t gamma_ids[] = {
	{ 15677, 6 },
	{ 15677, 8 },
	{ 0, 0 }
};

pci_ids_t gazel_ids[] = {
	{ 4277, 4144 },
	{ 4277, 4433 },
	{ 4277, 4434 },
	{ 0, 0 }
};

pci_ids_t gdth_ids[] = {
	{ 4377, 0 },
	{ 4377, 1 },
	{ 4377, 13 },
	{ 4377, 256 },
	{ 4377, 767 },
	{ 0, 0 }
};

pci_ids_t hamachi_ids[] = {
	{ 4888, 2321 },
	{ 0, 0 }
};

pci_ids_t hfc_pci_ids[] = {
	{ 2161, 65441 },
	{ 2161, 65442 },
	{ 4163, 1653 },
	{ 4177, 256 },
	{ 4431, 112 },
	{ 4431, 113 },
	{ 4431, 114 },
	{ 4431, 115 },
	{ 5015, 11216 },
	{ 5015, 45056 },
	{ 5015, 45062 },
	{ 5015, 45063 },
	{ 5015, 45064 },
	{ 5015, 45065 },
	{ 5015, 45066 },
	{ 5015, 45067 },
	{ 5015, 45068 },
	{ 5015, 45312 },
	{ 5073, 11217 },
	{ 5552, 11216 },
	{ 0, 0 }
};

pci_ids_t horizon_ids[] = {
	{ 4278, 4096 },
	{ 0, 0 }
};

pci_ids_t hp100_ids[] = {
	{ 4122, 5 },
	{ 4156, 4144 },
	{ 4156, 4145 },
	{ 4598, 274 },
	{ 0, 0 }
};

pci_ids_t hysdn_ids[] = {
	{ 4965, 36944 },
	{ 0, 0 }
};

pci_ids_t i810_tco_ids[] = {
	{ 2902, 9232 },
	{ 32902, 9248 },
	{ 32902, 9280 },
	{ 32902, 9292 },
	{ 0, 0 }
};

pci_ids_t i810_audio_ids[] = {
	{ 32902, 9237 },
	{ 32902, 9253 },
	{ 32902, 9285 },
	{ 32902, 9349 },
	{ 32902, 29077 },
	{ 0, 0 }
};

pci_ids_t i810_rng_ids[] = {
	{ 32902, 4400 },
	{ 32902, 9240 },
	{ 32902, 9256 },
	{ 0, 0 }
};

pci_ids_t ide_dma_ids[] = {
	{ 4130, 29705 },
	{ 4245, 1603 },
	{ 4281, 21017 },
	{ 0, 0 }
};

pci_ids_t ide_pci_ids[] = {
	{ 4107, 2 },
	{ 4107, 53249 },
	{ 4130, 29699 },
	{ 4130, 29705 },
	{ 4153, 21779 },
	{ 4162, 4096 },
	{ 4162, 4097 },
	{ 4162, 12320 },
	{ 4165, 50520 },
	{ 4165, 50721 },
	{ 4165, 54632 },
	{ 4181, 37168 },
	{ 4186, 3376 },
	{ 4186, 19760 },
	{ 4186, 19763 },
	{ 4186, 19768 },
	{ 4192, 257 },
	{ 4192, 26426 },
	{ 4192, 34922 },
	{ 4216, 258 },
	{ 4224, 50835 },
	{ 4245, 1600 },
	{ 4245, 1603 },
	{ 4245, 1606 },
	{ 4245, 1608 },
	{ 4245, 1609 },
	{ 4269, 261 },
	{ 4281, 21033 },
	{ 4355, 3 },
	{ 4355, 4 },
	{ 4358, 1377 },
	{ 4358, 1393 },
	{ 4454, 529 },
	{ 4497, 5 },
	{ 4497, 6 },
	{ 4497, 7 },
	{ 7649, 56361 },
	{ 32902, 2400 },
	{ 32902, 4654 },
	{ 32902, 4656 },
	{ 32902, 9233 },
	{ 32902, 9249 },
	{ 32902, 9290 },
	{ 32902, 9291 },
	{ 32902, 28688 },
	{ 32902, 28945 },
	{ 32902, 29081 },
	{ 32902, 30209 },
	{ 32902, 33994 },
	{ 37906, 25957 },
	{ 0, 0 }
};

pci_ids_t ide_tape_ids[] = {
	{ 4355, 3 },
	{ 4497, 5 },
	{ 0, 0 }
};

pci_ids_t igafb_ids[] = {
	{ 4330, 5762 },
	{ 0, 0 }
};

pci_ids_t ip2_ids[] = {
	{ 36366, 657 },
	{ 0, 0 }
};

pci_ids_t iph5526_ids[] = {
	{ 4222, 4 },
	{ 4222, 5 },
	{ 0, 0 }
};

pci_ids_t iphase_ids[] = {
	{ 4222, 8 },
	{ 0, 0 }
};

pci_ids_t lanstreamer_ids[] = {
	{ 4116, 24 },
	{ 0, 0 }
};

pci_ids_t lmc_ids[] = {
	{ 4113, 9 },
	{ 0, 0 }
};

pci_ids_t maestro_ids[] = {
	{ 4701, 256 },
	{ 4701, 6504 },
	{ 4701, 6520 },
	{ 0, 0 }
};

pci_ids_t matroxfb_base_ids[] = {
	{ 4139, 1305 },
	{ 4139, 1306 },
	{ 4139, 1307 },
	{ 4139, 1311 },
	{ 4139, 1312 },
	{ 4139, 1313 },
	{ 4139, 1317 },
	{ 4139, 4096 },
	{ 4139, 4097 },
	{ 32902, 4653 },
	{ 0, 0 }
};

pci_ids_t megaraid_ids[] = {
	{ 4126, 6496 },
	{ 4126, 36880 },
	{ 4126, 36960 },
	{ 0, 0 }
};

pci_ids_t natsemi_ids[] = {
	{ 4107, 32 },
	{ 0, 0 }
};

pci_ids_t ncr53c8xx_ids[] = {
	{ 4096, 1 },
	{ 4096, 2 },
	{ 4096, 3 },
	{ 4096, 4 },
	{ 4096, 6 },
	{ 4096, 11 },
	{ 4096, 12 },
	{ 4096, 13 },
	{ 4096, 15 },
	{ 4096, 18 },
	{ 4096, 143 },
	{ 0, 0 }
};

pci_ids_t ne2k_pci_ids[] = {
	{ 4176, 2368 },
	{ 4176, 23130 },
	{ 4285, 3636 },
	{ 4332, 32809 },
	{ 4358, 2342 },
	{ 4598, 5121 },
	{ 4803, 88 },
	{ 4803, 21912 },
	{ 18964, 20480 },
	{ 36398, 12288 },
	{ 0, 0 }
};

pci_ids_t niccy_ids[] = {
	{ 4711, 4118 },
	{ 0, 0 }
};

pci_ids_t nicstar_ids[] = {
	{ 4381, 1 },
	{ 0, 0 }
};

pci_ids_t nj_s_ids[] = {
	{ 57689, 1 },
	{ 0, 0 }
};

pci_ids_t nj_u_ids[] = {
	{ 57689, 1 },
	{ 0, 0 }
};

pci_ids_t nm256_audio_ids[] = {
	{ 4296, 32773 },
	{ 4296, 32774 },
	{ 0, 0 }
};

pci_ids_t ns558_ids[] = {
	{ 4354, 28674 },
	{ 4701, 6505 },
	{ 21299, 51712 },
	{ 0, 0 }
};

pci_ids_t olympic_ids[] = {
	{ 4116, 62 },
	{ 0, 0 }
};

pci_ids_t osb4_ids[] = {
	{ 4454, 512 },
	{ 0, 0 }
};

pci_ids_t parport_pc_ids[] = {
	{ 4277, 36944 },
	{ 4358, 1670 },
	{ 4895, 4112 },
	{ 4895, 4113 },
	{ 4895, 4114 },
	{ 4895, 4128 },
	{ 4895, 4129 },
	{ 4895, 4148 },
	{ 4895, 4149 },
	{ 4895, 4150 },
	{ 4895, 8208 },
	{ 4895, 8209 },
	{ 4895, 8210 },
	{ 4895, 8224 },
	{ 4895, 8225 },
	{ 4895, 8256 },
	{ 4895, 8257 },
	{ 4895, 8258 },
	{ 4895, 8288 },
	{ 4895, 8289 },
	{ 4895, 8290 },
	{ 5127, 32768 },
	{ 5127, 32770 },
	{ 5127, 32771 },
	{ 5127, 34816 },
	{ 5129, 29032 },
	{ 5129, 29288 },
	{ 5339, 8480 },
	{ 5522, 1922 },
	{ 5522, 1923 },
	{ 0, 0 }
};

pci_ids_t pcilynx_ids[] = {
	{ 4172, 32768 },
	{ 0, 0 }
};

pci_ids_t pcnet32_ids[] = {
	{ 4130, 8192 },
	{ 4130, 8193 },
	{ 0, 0 }
};

pci_ids_t pdc202xx_ids[] = {
	{ 4186, 3376 },
	{ 4186, 19760 },
	{ 4186, 19763 },
	{ 4186, 19768 },
	{ 0, 0 }
};

pci_ids_t piix_ids[] = {
	{ 32902, 4654 },
	{ 32902, 4656 },
	{ 32902, 9233 },
	{ 32902, 9249 },
	{ 32902, 9290 },
	{ 32902, 9291 },
	{ 32902, 28688 },
	{ 32902, 28945 },
	{ 32902, 29081 },
	{ 32902, 30209 },
	{ 32902, 33994 },
	{ 0, 0 }
};

pci_ids_t pm2fb_ids[] = {
	{ 4172, 15623 },
	{ 15677, 7 },
	{ 15677, 9 },
	{ 0, 0 }
};
#if 0
pci_ids_t pmc551_ids[] = {
	{ 4528,PCI_DEVICE_ID_V3_SEMI_V370PDC
	{ 0, 0 }
};
#endif
pci_ids_t qlogicfc_ids[] = {
	{ 4215, 8448 },
	{ 4215, 8704 },
	{ 0, 0 }
};

pci_ids_t qlogicisp_ids[] = {
	{ 4215, 4128 },
	{ 0, 0 }
};

pci_ids_t radio_maestro_ids[] = {
	{ 4701, 6504 },
	{ 4701, 6520 },
	{ 0, 0 }
};

pci_ids_t radio_maxiradio_ids[] = {
	{ 20550, 4097 },
	{ 0, 0 }
};

pci_ids_t rio_ids[] = {
	{ 4555, 8192 },
	{ 4555, 32768 },
	{ 0, 0 }
};

pci_ids_t rivafb_ids[] = {
	{ 4318, 32 },
	{ 4318, 40 },
	{ 4318, 41 },
	{ 4318, 44 },
	{ 4318, 45 },
	{ 4318, 160 },
	{ 4318, 256 },
	{ 4318, 257 },
	{ 4318, 259 },
	{ 4318, 272 },
	{ 4318, 273 },
	{ 4318, 275 },
	{ 4318, 336 },
	{ 4318, 337 },
	{ 4318, 339 },
	{ 4818, 24 },
	{ 0, 0 }
};

pci_ids_t rrunner_ids[] = {
	{ 4623, 1 },
	{ 0, 0 }
};

pci_ids_t rz1000_ids[] = {
	{ 4162, 4096 },
	{ 4162, 4097 },
	{ 0, 0 }
};

pci_ids_t S3triofb_ids[] = {
	{ 21299, 34833 },
	{ 0, 0 }
};

#if 0
pci_ids_t saa9730_ids[] = {
	{ 4401,PCI_DEVICE_ID_PHILIPS_SAA9730 },
	{ 0, 0 }
};
#endif

pci_ids_t sdladrv_ids[] = {
	{ 4528, 2 },
	{ 0, 0 }
};

pci_ids_t sedlbauer_ids[] = {
	{ 57689, 2 },
	{ 0, 0 }
};

pci_ids_t sis5513_ids[] = {
	{ 4153, 1328 },
	{ 4153, 1344 },
	{ 4153, 1568 },
	{ 4153, 1584 },
	{ 4153, 1840 },
	{ 4153, 21777 },
	{ 4153, 21905 },
	{ 4153, 21911 },
	{ 4153, 22016 },
	{ 0, 0 }
};

pci_ids_t sis900_ids[] = {
	{ 4153, 2304 },
	{ 4153, 28694 },
	{ 0, 0 }
};

pci_ids_t sisfb_ids[] = {
	{ 4153, 768 },
	{ 4153, 21248 },
	{ 4153, 25344 },
	{ 4153, 29440 },
	{ 0, 0 }
};

pci_ids_t sk98lin_ids[] = {
	{ 4424, 17152 },
	{ 0, 0 }
};

pci_ids_t skfp_ids[] = {
	{ 4424, 16384 },
	{ 0, 0 }
};

pci_ids_t sl82c105_ids[] = {
	{ 4269, 1381 },
	{ 0, 0 }
};

pci_ids_t sonicvibes_ids[] = {
	{ 21299, 51712 },
	{ 0, 0 }
};

pci_ids_t specialix_ids[] = {
	{ 4555, 8192 },
	{ 0, 0 }
};

pci_ids_t stallion_ids[] = {
	{ 4107, 53249 },
	{ 0, 0 }
};

pci_ids_t starfire_ids[] = {
	{ 36868, 26901 },
	{ 0, 0 }
};

pci_ids_t stradis_ids[] = {
	{ 4401, 28998 },
	{ 0, 0 }
};

pci_ids_t sundance_ids[] = {
	{ 4486, 4098 },
	{ 5104, 513 },
	{ 0, 0 }
};

pci_ids_t sungem_ids[] = {
	{ 4203, 33 },
	{ 4238, 4353 },
	{ 4238, 11181 },
	{ 0, 0 }
};

pci_ids_t sunhme_ids[] = {
	{ 4113, 37 },
	{ 4238, 4097 },
	{ 0, 0 }
};

pci_ids_t sx_ids[] = {
	{ 4555, 8192 },
	{ 0, 0 }
};

pci_ids_t sym53c8xx_ids[] = {
	{ 4096, 1 },
	{ 4096, 6 },
	{ 4096, 11 },
	{ 4096, 12 },
	{ 4096, 13 },
	{ 4096, 15 },
	{ 4096, 32 },
	{ 4096, 33 },
	{ 0, 0 }
};

pci_ids_t synclink_ids[] = {
	{ 5056, 16 },
	{ 0, 0 }
};

pci_ids_t t1pci_ids[] = {
	{ 4676, 4608 },
	{ 0, 0 }
};

pci_ids_t tdfxfb_ids[] = {
	{ 4634, 3 },
	{ 4634, 5 },
	{ 4634, 9 },
	{ 0, 0 }
};

pci_ids_t telespci_ids[] = {
	{ 4574, 24864 },
	{ 0, 0 }
};

pci_ids_t tgafb_ids[] = {
	{ 4113, 4 },
	{ 0, 0 }
};

pci_ids_t tlan_ids[] = {
	{ 3601, 44594 },
	{ 3601, 44596 },
	{ 3601, 44597 },
	{ 3601, 44608 },
	{ 3601, 44611 },
	{ 3601, 45073 },
	{ 3601, 45074 },
	{ 3601, 45104 },
	{ 3601, 61744 },
	{ 3601, 61776 },
	{ 4237, 18 },
	{ 4237, 19 },
	{ 4237, 20 },
	{ 0, 0 }
};

pci_ids_t tmspci_ids[] = {
	{ 3601, 1288 },
	{ 4279, 13200 },
	{ 4314, 1288 },
	{ 4424, 16896 },
	{ 0, 0 }
};

pci_ids_t toshoboe_ids[] = {
	{ 4473,1793 },
	{ 4473,3329 },
	{ 0, 0 }
};

pci_ids_t tpam_ids[] = {
	{ 4334, 16416 },
	{ 0, 0 }
};

pci_ids_t trident_ids[] = {
	{ 4131, 8192 },
	{ 4131, 8193 },
	{ 4153, 28696 },
	{ 4281, 5427 },
	{ 4281, 21585 },
	{ 4281, 28929 },
	{ 0, 0 }
};

pci_ids_t tulip_ids[] = {
	{ 4113, 2 },
	{ 4113, 9 },
	{ 4113, 20 },
	{ 4113, 25 },
	{ 4170, 2433 },
	{ 4170, 10100 },
	{ 4313, 1298 },
	{ 4313, 1329 },
	{ 4371, 4630 },
	{ 4371, 4631 },
	{ 4371, 38161 },
	{ 4525, 2 },
	{ 4525, 49429 },
	{ 4598, 39041 },
	{ 4699, 5120 },
	{ 4738, 37120 },
	{ 4738, 37122 },
	{ 4887, 2433 },
	{ 4887, 2437 },
	{ 4887, 6533 },
	{ 5073, 43778 },
	{ 5073, 43779 },
	{ 32902, 57 },
	{ 0, 0 }
};

pci_ids_t via_rhine_ids[] = {
	{ 4358, 12355 },
	{ 4358, 12389 },
	{ 4358, 24832 },
	{ 0, 0 }
};

pci_ids_t via82cxxx_ids[] = {
	{ 4358, 1414 },
	{ 4358, 1430 },
	{ 4358, 1670 },
	{ 4358, 12404 },
	{ 4358, 33329 },
	{ 0, 0 }
};

pci_ids_t via82cxxx_audio_ids[] = {
	{ 4358, 12376 },
	{ 0, 0 }
};

pci_ids_t w6692_ids[] = {
	{ 1653, 5890 },
	{ 4176, 26258 },
	{ 0, 0 }
};

pci_ids_t wdt_pci_ids[] = {
	{ 18767, 8896 },
	{ 0, 0 }
};

pci_ids_t winbond_840_ids[] = {
	{ 4176, 2112 },
	{ 4598, 8209 },
	{ 0, 0 }
};

pci_ids_t yellowfin_ids[] = {
	{ 4096, 1793 },
	{ 4096, 1794 },
	{ 0, 0 }
};

pci_ids_t ymfpci_ids[] = {
	{ 4211, 4 },
	{ 4211, 10 },
	{ 4211, 12 },
	{ 4211, 13 },
	{ 4211, 16 },
	{ 4211, 18 },
	{ 0, 0 }
};

pci_ids_t zatm_ids[] = {
	{ 4499, 1 },
	{ 0, 0 }
};

pci_ids_t zoran_ids[] = {
	{ 4574, 24864 },
	{ 32902, 4653 },
	{ 0, 0 }
};

module_list_t module_list[] = {
	{ "3c59x", _3c59x_ids },
	{ "8139too", _8139too_ids },
	{ "abyss", abyss_ids },
	{ "acenic", acenic_ids },
	{ "aec62xx", aec62xx_ids },
	{ "agpart", agpart_ids },
	{ "aic5800", aic5800_ids },
	{ "alim15x3", alim15x3_ids },
	{ "ambassador", ambassador_ids },
	{ "applicom", applicom_ids },
	{ "aty128fb", aty128fb_ids },
	{ "avm", avm_ids },
	{ "b1pci", b1pci_ids },
	{ "bkm_a4t", bkm_a4t_ids },
	{ "bkm_a8", bkm_a8_ids },
	{ "bttv", bttv_ids },
	{ "buz", buz_ids },
	{ "c4", c4_ids },
	{ "cciss", cciss_ids },
	{ "chipsfb", chipsfb_ids },
	{ "clgenfb", clgenfb_ids },
	{ "cmpci", cmpci_ids },
	{ "cpqarray", cpqarray_ids },
	{ "cs4281", cs4281_ids },
	{ "cs461x", cs461x_ids },
	{ "cyber2000fb", cyber2000fb_ids },
	{ "cyclades", cyclades_ids },
	{ "defxx", defxx_ids },
	{ "dgrs", dgrs_ids },
	{ "diva", diva_ids },
	{ "dmfe", dmfe_ids },
	{ "dmx3191d", dmx3191d_ids },
	{ "dscc4", dscc4_ids },
	{ "eepro100", eepro100_ids },
	{ "elsa", elsa_ids },
	{ "emu10k1", emu10k1_ids },
	{ "eni", eni_ids },
	{ "epca", epca_ids },
	{ "epic100", epic100_ids },
	{ "es1370", es1370_ids },
	{ "es1371", es1371_ids },
	{ "esssolo1", esssolo1_ids },
	{ "fdomain", fdomain_ids },
	{ "fealnx", fealnx_ids },
	{ "firestream", firestream_ids },
	{ "fore_200e", fore_200e_ids },
	{ "gamma", gamma_ids },
	{ "gazel", gazel_ids },
	{ "gdth", gdth_ids },
	{ "hamachi", hamachi_ids },
	{ "hfc_pci", hfc_pci_ids },
	{ "horizon", horizon_ids },
	{ "hp100", hp100_ids },
	{ "hysdn", hysdn_ids },
	{ "i810-tco", i810_tco_ids },
	{ "i810_audio", i810_audio_ids },
	{ "i810_rng", i810_rng_ids },
	{ "ide-dma", ide_dma_ids },
	{ "ide-pci", ide_pci_ids },
	{ "ide-tape", ide_tape_ids },
	{ "igafb", igafb_ids },
	{ "ip2", ip2_ids },
	{ "iph5526", iph5526_ids },
	{ "iphase", iphase_ids },
	{ "lanstreamer", lanstreamer_ids },
	{ "lmc", lmc_ids },
	{ "matroxfb_base", matroxfb_base_ids },
	{ "megaraid", megaraid_ids },
	{ "natsemi", natsemi_ids },
	{ "ncr53c8xx", ncr53c8xx_ids },
	{ "ne2k-pci", ne2k_pci_ids },
	{ "niccy", niccy_ids },
	{ "nicstar", nicstar_ids },
	{ "nj_s", nj_s_ids },
	{ "nj_u", nj_u_ids },
	{ "nm256_audio", nm256_audio_ids },
	{ "ns558", ns558_ids },
	{ "olympic", olympic_ids },
	{ "osb4", osb4_ids },
	{ "parport_pc", parport_pc_ids },
	{ "pcilynx", pcilynx_ids },
	{ "pcnet32", pcnet32_ids },
	{ "pdc202xx", pdc202xx_ids },
	{ "piix", piix_ids },
	{ "pm2fb", pm2fb_ids },
//	{ "pmc551", pmc551_ids },
	{ "qlogicfc", qlogicfc_ids },
	{ "qlogicisp", qlogicisp_ids },
	{ "radio-maestro", radio_maestro_ids },
	{ "radio-maxiradio", radio_maxiradio_ids },
	{ "rio", rio_ids },
	{ "rivafb", rivafb_ids },
	{ "rrunner", rrunner_ids },
	{ "rz1000", rz1000_ids },
	{ "S3triofb", S3triofb_ids },
//	{ "saa9730", saa9730_ids },
	{ "sdladrv", sdladrv_ids },
	{ "sedlbauer", sedlbauer_ids },
	{ "sis5513", sis5513_ids },
	{ "sis900", sis900_ids },
	{ "sisfb", sisfb_ids },
	{ "sk98lin", sk98lin_ids },
	{ "skfp", skfp_ids },
	{ "sl82c105", sl82c105_ids },
	{ "sonicvibes", sonicvibes_ids },
	{ "specialix", specialix_ids },
	{ "stallion", stallion_ids },
	{ "starfire", starfire_ids },
	{ "stradis", stradis_ids },
	{ "sundance", sundance_ids },
	{ "sungem", sungem_ids },
	{ "sunhme", sunhme_ids },
	{ "sx", sx_ids },
	{ "sym53c8xx", sym53c8xx_ids },
	{ "synclink", synclink_ids },
	{ "t1pci", t1pci_ids },
	{ "tdfxfb", tdfxfb_ids },
	{ "telespci", telespci_ids },
	{ "tgafb", tgafb_ids },
	{ "tlan", tlan_ids },
	{ "tmspci", tmspci_ids },
	{ "toshoboe", toshoboe_ids },
	{ "tpam", tpam_ids },
	{ "trident", trident_ids },
	{ "tulip", tulip_ids },
	{ "via-rhine", via_rhine_ids },
	{ "via82cxxx", via82cxxx_ids },
	{ "via82cxxx_audio", via82cxxx_audio_ids },
	{ "w6692", w6692_ids },
	{ "wdt_pci", wdt_pci_ids },
	{ "winbond-840", winbond_840_ids },
	{ "yellowfin", yellowfin_ids },
	{ "ymfpci", ymfpci_ids },
	{ "zatm", zatm_ids },
	{ "zoran", zoran_ids },
	{ NULL, 0 }
};
