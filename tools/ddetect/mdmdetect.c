/*
 * file : mdmdetect.c 
 * purpose : detect modems for the Debian/GNU Linux installer
 * author : David Whedon <dwhedon@gordian.com>
 */


#include <stdio.h>
#include <discover.h>
#include <unistd.h>

#include "mdmdetect.h"
#include "ddetect.h"

int
main (int argc, char *argv[])
{
  struct bus_lst bus = { 0 };
  struct modem_info *modem = (struct modem_info *) NULL;
  int i = 0;

  ddetect_getopts (argc, argv);

  sync ();

  while (lst[i].vendor)
    {
      lst[i].next = &lst[++i];
    }
  lst[i - 1].next = NULL;


  bus.pci = pci_detect (lst);
  bus.usb = usb_detect (lst);
  bus.serial = serial_detect (lst);
  if (!passive_detection)
      bus.isa = isa_detect (lst);
  if (((modem = modem_detect (&bus)) != NULL) && (debug == 1))
    {
      printf ("bus:device:speed:module\n");
    }				/*endif */
  for (; modem; modem = modem->next)
    {
      printf ("%s:%s:%ld:%s\n",
	      bus2str (modem->bus),
	      modem->device, modem->speed, modem->module);
    }				/*next modem */

  return 0;
}
