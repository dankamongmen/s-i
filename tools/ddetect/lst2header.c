/*
   file - lst2header.c
   purpose - generate a header file suitable for ddetect binaries.
 */



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "discover.h"


#define PATH_ISA_LST "lst/isa.lst"
#define PATH_PCI_LST "lst/pci.lst"
#define PATH_PCMCIA_LST "lst/pcmcia.lst"
#define PATH_USB_LST "lst/usb.lst"

static void
usage (char *basename)
{
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "%s [options]\n", basename);
  fprintf (stderr, "-e select ethernet cards\n");
  fprintf (stderr, "-c select cdroms\n");
  fprintf (stderr, "-s select sound cards\n");
  fprintf (stderr, "-m select modems\n");
  exit (1);
}


main (int argc, char *argv[])
{
  int verbose = 1;
  struct cards_lst *lst = (struct cards_lst *) NULL, *ptr;
  struct bus_lst *bus = (struct bus_lst *) NULL;
  struct ethernet_info *ethernet = (struct ethernet_info *) NULL;
  int ethernet_flag = 0, cdrom_flag = 0, sound_flag = 0, modem_flag = 0;
  int i = 1;
  time_t t;
  while (argv[i] != NULL)
    {
      if (argv[i][0] != '-')
	usage (argv[0]);
      switch (argv[i][1])
	{
	case 'e':
	  ethernet_flag = 1;
	  break;
	case 'c':
	  cdrom_flag = 1;
	  break;
	case 's':
	  sound_flag = 1;
	  break;
	case 'm':
	  modem_flag = 1;
	  break;
	default:
	  usage (argv[0]);
	}
      i++;
    }

  /* Initialize hardware device list                                  */
  lst = init_lst (PATH_PCI_LST, PATH_PCMCIA_LST, PATH_USB_LST);

  time (&t);
  printf ("/* \tThis file generated on %s", ctime (&t));
  printf ("\tby lst2header, part of ddetect (hardware detection\n");
  printf ("\tutilities for the Debian GNU/Linux installer)\n");
  printf
    ("\tThis header file includes information necessary to detect various:\n");
  if (ethernet_flag)
    printf ("\tethernet cards\n");
  if (cdrom_flag)
    printf ("\tcdrom drives\n");
  if (sound_flag)
    printf ("\tsound cards\n");
  if (modem_flag)
    printf ("\tmodems\n");

  printf ("*/\n\n\n\n");

  printf ("static char unknown_str[]=\"unknown\";\n\n");
  
  printf ("struct cards_lst lst[] = {\n");
  ptr = lst;
  while (ptr)
    {
      if ((ptr->type == ETHERNETCARD && ethernet_flag)
	  || (ptr->type == CDROM && cdrom_flag)
	  || (ptr->type == SOUNDCARD && sound_flag)
	  || (ptr->type == MODEM && modem_flag))
	{
	  printf
	    ("{ unknown_str, \tunknown_str, \t\"%s\", \t%d,\t\"%s\", \t%d, \t%d, \t%d, \tNULL},\n /* %s %s */\n\n",
	     (ptr->modulename ? ptr->modulename : "unknown"), ptr->bus,
	     (ptr->dev_id ? ptr->dev_id : "unknown"), ptr->long_id, ptr->type,
	     ptr->options, ptr->model, ptr->vendor);
	}
      ptr = ptr->next;
    }
  printf ("{NULL}};\n");

  return 0;

}
