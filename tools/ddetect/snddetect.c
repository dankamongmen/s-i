/*
 * file : snddetect 
 * purpose : detect sound cards for the Debian/GNU Linux installer
 * author : David Whedon <dwhedon@gordian.com>
 */


#include <stdio.h>
#include <detect.h>
#include <unistd.h>

#include "snddetect.h"
#include "ddetect.h"

int
main (int argc, char *argv[])
{
  struct bus_lst bus = { 0 };
  struct soundcard_info *sound = (struct soundcard_info *) NULL;
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

  /*  
     bus.isa = isa_detect(lst);
     bus.pcmcia = pcmcia_detect(lst);
     bus.usb = usb_detect(lst);
     bus.ide = ide_detect();
     bus.scsi = scsi_detect();
     bus.parallel = parallel_detect();
     bus.serial = serial_detect(lst);
   */


  sound = soundcard_detect (&bus);

  for (; sound; sound = sound->next)
    {
      printf ("%s\t%s\n", sound->model, sound->module);
    }
  return 0;
}
