/*
 * file : ddetect 
 * purpose : detect ethernet cards for the Debian/GNU Linux installer
 * author : David Whedon <dwhedon@gordian.com>
 */


#include <stdio.h>
#include <detect.h>
#include <unistd.h>

#include "ethdetect.h"
#include "ddetect.h"

int
main (int argc, char *argv[])
{
  struct bus_lst bus = { 0 };
  struct ethernet_info *ethernet = (struct ethernet_info *) NULL;
  int i = 0;

  ddetect_getopts (argc, argv);

  sync ();

  while (lst[i].vendor)
    {
      lst[i].next = &lst[++i];
    }
  lst[i - 1].next = NULL;


  bus.pci = pci_detect (lst);
  if (!passive_detection)
    bus.isa = isa_detect (lst);

  bus.pcmcia = pcmcia_detect (lst);


  if (((ethernet = ethernet_detect (&bus)) != NULL) && (debug == 1))
    {
      printf ("\n%s:%s:%s:%s\n", "bus", "vendor", "model", "module");
    }				/*endif */
  for (; ethernet; ethernet = ethernet->next)
    {
      printf ("%s:%s:%s:%s\n",
	      bus2str (ethernet->bus),
	      ethernet->vendor, ethernet->model, ethernet->module);
    }				/*next ethernet */



  return 0;
}
