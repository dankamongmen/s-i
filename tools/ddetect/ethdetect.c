/*
 * file : ddetect 
 * purpose : detect ethernet cards for the Debian/GNU Linux installer
 * author : David Whedon <dwhedon@gordian.com>
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <detect.h>

#include "ddetect.h"
#include "ethdetect.h"

int
main (int argc, char *argv[])
{
  struct bus_lst bus = { 0 };
  struct ethernet_info *ethernet = (struct ethernet_info *) NULL;
  int i, j, num_cards;

  struct cards_lst *lst = NULL;
  char *unknown_str = "unknown";
  char *ignore_str = "ignore";

  ddetect_getopts (argc, argv);
  num_cards = 0;
  for (i = 0; pci_modules[i].name; i++)
    for (j = 0; pci_modules[i].id[j].long_id; j++)
      num_cards++;

  if (!passive_detection)
    {
      for (i = 0; isa_modules[i].name; i++)
	for (j = 0; isa_modules[i].id[j].dev_id; j++)
	  num_cards++;
    }

  lst = malloc (sizeof (struct cards_lst) * num_cards);

  num_cards = 0;

  /* initialize list of pci cards */
  for (i = 0; pci_modules[i].name; i++)
    for (j = 0; pci_modules[i].id[j].long_id; j++)
      {
	lst[num_cards].vendor = unknown_str;
	lst[num_cards].model = unknown_str;
	lst[num_cards].modulename = pci_modules[i].name;
	lst[num_cards].bus = PCI;
	lst[num_cards].dev_id = ignore_str;
	lst[num_cards].long_id = pci_modules[i].id[j].long_id;
	lst[num_cards].type = ETHERNETCARD;
	lst[num_cards].options = 0;
	lst[num_cards].next = &lst[++num_cards];
      }


  if (!passive_detection)
    {
      /* initialize list of isa cards */
      for (i = 0; isa_modules[i].name; i++)
	for (j = 0; isa_modules[i].id[j].dev_id; j++)
	  {
	    lst[num_cards].vendor = unknown_str;
	    lst[num_cards].model = unknown_str;
	    lst[num_cards].modulename = isa_modules[i].name;
	    lst[num_cards].bus = ISA;
	    lst[num_cards].dev_id = isa_modules[i].id[j].dev_id;
	    lst[num_cards].long_id = 0;
	    lst[num_cards].type = ETHERNETCARD;
	    lst[num_cards].options = 0;
	    lst[num_cards].next = &lst[++num_cards];
	  }
    }

  lst[num_cards - 1].next = NULL;


  sync ();
  bus.pci = pci_detect (lst);
  if (!passive_detection)
    bus.isa = isa_detect (lst);

  bus.pcmcia = pcmcia_detect (lst);


  if (((ethernet = ethernet_detect (&bus)) != NULL) && (debug == 1))
    {
      printf ("\nbus:module:(id)\n");
    }
  for (; ethernet; ethernet = ethernet->next)
    {
      printf ("%s:%s:(0x%.8lx)\n",
	      bus2str (ethernet->bus), ethernet->module, ethernet->long_id);
    }


  return 0;
}
