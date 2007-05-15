#include <parted/parted.h>
#include <stdio.h>

main (int argc, char **argv)
{
  PedDevice *device;
  PedDisk *disk;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s DEVICE\n", argv[0]);
      exit (1);
    }

  device = ped_device_get (argv[1]);
  if (!device)
    exit (1);

  disk = ped_disk_new (device);
  if (!disk)
    exit (1);

  printf ("%s\n", disk->type->name);
}
