/*
 * file : diskdetect.c 
 * purpose : detect ide and scsi hard disks for the Debian/GNU Linux installer
 * author : David Whedon <dwhedon@gordian.com>
 */


#include <stdio.h>
#include <detect.h>
#include <unistd.h>


static void
usage (char *basename)
{
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "diskdetect [options]\n");
  fprintf (stderr, "-v\tverbose\n");
  exit (1);
}


int
main (int argc, char *argv[])
{
  struct disk_info *disk = (struct disk_info *) NULL;
  struct bus_lst bus = { 0 };
  int i = 1;

  while (argv[i] != NULL)
    {
      if (argv[i][0] != '-')
	usage (argv[0]);
      switch (argv[i][1])
	{
	case 'v':
	  debug = 1;
	  break;
	default:
	  usage (argv[0]);
	}
      i++;
    }


  sync ();

  bus.ide = ide_detect ();
  bus.scsi = scsi_detect ();

  if (((disk = disk_detect (&bus)) != NULL) && (debug == 1))
    {
      printf ("\n%s:%s:%s:%s:%s:%s:%s:%s\n",
	      "bus",
	      "vendor",
	      "model",
	      "device", "size(512k-blocks)", "heads", "sectors", "cylinders");
    }				/*endif */
  for (; disk; disk = disk->next)
    {
      printf ("%s:%s:%s:%s:%d:%d:%d:%d\n",
	      bus2str (disk->bus),
	      disk->vendor,
	      disk->model,
	      disk->device,
	      disk->size, disk->heads, disk->sectors, disk->cylinders);
    }				/*next disk */

  return 0;
}
