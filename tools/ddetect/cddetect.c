/*
 * file : cddetect 
 * purpose : detect cdrom for the Debian/GNU Linux installer
 * author : David Whedon <dwhedon@gordian.com>
 */


#include <stdio.h>
#include <detect.h>
#include <unistd.h>

#include "cddetect.h"
#include "ddetect.h"

int
main (int argc, char *argv[])
{
  struct bus_lst bus = { 0 };
  struct cdrom_info *cdrom = (struct cdrom_info *) NULL;
  int i = 0;

  ddetect_getopts (argc, argv);

  sync ();

  while (lst[i].vendor)
    {
      lst[i].next = &lst[++i];
    }
  lst[i - 1].next = NULL;

  bus.ide = ide_detect ();
  bus.scsi = scsi_detect ();
  if (!passive_detection)
    bus.isa = isa_detect (lst);


  if (((cdrom = cdrom_detect (&bus)) != NULL) && (debug == 1))
    {
      printf ("\nbus:model:device\n");
    }				/*endif */
  for (; cdrom; cdrom = cdrom->next)
    {
      printf ("%s:%s:%s\n",
	      bus2str (cdrom->bus),
	      cdrom->model, cdrom->device);
    }				/*next cdrom */




  return 0;
}
