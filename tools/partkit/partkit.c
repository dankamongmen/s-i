/* 
   partkit.c - Module to partition devices for debian-installer.
   Author - David Whedon

   Copyright (C) 2000  David Whedon <dwhedon@debian.org>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   
*/

#define MEGABYTE (1024 * 1024)
#define MEGABYTE_SECTORS (MEGABYTE / 512)
#define _(ARG) ARG
#define PARTKIT_TABLE_SIZE 2048
#define PARTKIT_PART_LIST_SIZE 128

/* I can't understand why they did removed it. */
#define PED_PARTITION_PRIMARY 0

#include <string.h>
#include <parted/parted.h>
#include <debconfclient.h>
static struct debconfclient *client;


void
partkit_error (int rv)
{
  client->command (client, "input", "high", "partkit/error", NULL);
  client->command (client, "go", NULL);
  if (rv)
    exit (rv);
}

char *
debconf_input (char *priority, char *template)
{
  client->command (client, "fset", template, "seen", "false", NULL);
  client->command (client, "input", priority, template, NULL);
  client->command (client, "go", NULL);
  client->command (client, "get", template, NULL);
  return client->value;
}


/*
 * return a malloc'ed string properly formatted for inclusion in the debconf
 * 'Choices' field.  The string indicated what operations we are able to
 * perform on the specified device (remove partitions, delete resize, etc.).
 * Always end the string with the special option "exit"
 */
char *
partkit_get_operations (PedDevice * dev)
{
  char *ops;

  if ((ops = malloc (256)) == NULL)
    partkit_error (1);
  snprintf (ops, 256, "create partitions, delete partitions, exit");
  return ops;
}

/*
 * return a malloc'ed string properly formated for debconf containing the
 * partition table for the device specified.  If an error occurs return a
 * string indicating an error.
 */

char *
partkit_get_table (PedDevice ** dev)
{
  PedDisk *disk;
  PedPartition *part;
  PedPartitionFlag flag;
  int first_flag;
  int has_extended;
  int has_name;
  char *table, *ptr;

  if ((table = malloc (PARTKIT_TABLE_SIZE + 1)) == NULL)
    goto error;

  ptr = table;

  disk = ped_disk_new (*dev);

  if (!disk)
    goto error;

  ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
		   _("Disk geometry for %s: 0.000-%.3f megabytes\n"),
		   disk->dev->path,
		   (disk->dev->length - 1) * 1.0 / MEGABYTE_SECTORS);
  ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
		   _("Disk label type: %s\n"), disk->type->name);

  has_extended = ped_disk_type_check_feature (disk->type,
					      PED_DISK_TYPE_EXTENDED);
  has_name = ped_disk_type_check_feature (disk->type,
					  PED_DISK_TYPE_PARTITION_NAME);

  ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
		   _("Minor    Start       End     "));

  if (has_extended)
    ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
		     _("Type      "));
  ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
		   _("Filesystem  "));

  if (has_name)
    ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
		     _("Name                  "));

  ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table), _("Flags"));
  ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table), "\n");

  for (part = ped_disk_next_partition (disk, NULL); part;
       part = ped_disk_next_partition (disk, part))
    {

      if (part->type != PED_PARTITION_PRIMARY
	  && part->type != PED_PARTITION_LOGICAL
	  && part->type != PED_PARTITION_EXTENDED)
	continue;

      ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
		       "%-5d ", part->num);

      ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
		       "%10.3f %10.3f  ",
		       (int) part->geom.start * 1.0 / MEGABYTE_SECTORS,
		       (int) part->geom.end * 1.0 / MEGABYTE_SECTORS);

      if (has_extended)
	ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
			 "%-9s ", ped_partition_type_get_name (part->type));

      ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
		       "%-12s", part->fs_type ? part->fs_type->name : "");

      if (has_name)
	ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
			 "%-22s", ped_partition_get_name (part));

      first_flag = 1;
      for (flag = ped_partition_flag_next (0); flag;
	   flag = ped_partition_flag_next (flag))
	{
	  if (ped_partition_get_flag (part, flag))
	    {
	      if (first_flag)
		first_flag = 0;
	      else
		ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
				 ", ");
	      ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table),
			       _(ped_partition_flag_get_name (flag)));
	    }
	}
      ptr += snprintf (ptr, PARTKIT_TABLE_SIZE - (ptr - table), "\n");
    }

  ped_disk_destroy (disk);
  if ((ptr - table) > PARTKIT_TABLE_SIZE)
    goto error;
  return table;

error:
  partkit_error (0);
  snprintf (table, PARTKIT_TABLE_SIZE, "<Partition Table Not Available>");
  return table;
}

/*
 * return a list of partitions on dev
 */


char *
partkit_get_partitions (PedDevice * dev)
{

  PedDisk *disk;
  PedPartition *part;
  char *part_list = NULL, *ptr;

  if ((part_list = malloc (PARTKIT_PART_LIST_SIZE + 1)) == NULL)
    goto error;

  ptr = part_list;

  disk = ped_disk_new (dev);

  if (!disk)
    goto error;

  for (part = ped_disk_next_partition (disk, NULL); part;
       part = ped_disk_next_partition (disk, part))
    {

      if (part->type != PED_PARTITION_PRIMARY
	  && part->type != PED_PARTITION_LOGICAL
	  && part->type != PED_PARTITION_EXTENDED)
	continue;

      ptr += snprintf (ptr, PARTKIT_PART_LIST_SIZE - (ptr - part_list),
		       "%-5d ", part->num);
    }

  ped_disk_destroy (disk);

  if ((ptr - part_list) > PARTKIT_PART_LIST_SIZE)
    goto error;
  return part_list;

error:
  if (part_list)
    free (part_list);
  partkit_error (0);
  return NULL;
}


int
partkit_create (PedDevice * dev)
{
  PedDisk *disk;
  PedPartition *part;
  PedPartitionType part_type;
  PedSector start, end;
  PedConstraint *constraint;
  char *ptr, *endptr = NULL;
  char *table;

  disk = ped_disk_new (dev);
  if (!disk)
    goto error;
  constraint = ped_constraint_any (disk->dev);
  if (!constraint)
    goto error_close_disk;

  client->command (client, "subst", "partkit/create_part_type", "choices",
		   "primary, logical, extended", NULL);

  table = partkit_get_table (&dev);
  client->command (client, "subst", "partkit/create_part_type", "table",
		   table, NULL);

  ptr = debconf_input ("critical", "partkit/create_part_type");

  switch (ptr[0])
    {
    case 'p':
      part_type = PED_PARTITION_PRIMARY;
      break;
    case 'e':
      part_type = PED_PARTITION_EXTENDED;
      break;
    case 'l':
      part_type = PED_PARTITION_LOGICAL;
      break;
    default:
      goto error_destroy_constraint;
    }

  client->command (client, "subst", "partkit/create_start",
		   "table", table, NULL);

  do
    {
      ptr = debconf_input ("critical", "partkit/create_start");
      start = (strtod (ptr, &endptr)) * MEGABYTE_SECTORS;
      if (ptr == endptr)
	{
	  client->command (client, "input", "high", "partkit/create_bad_num",
			   NULL);
	  client->command (client, "go", NULL);
	  endptr = NULL;
	}
      else
	client->command (client, "subst", "partkit/create_confirm",
			 "start", start, NULL);
    }
  while (endptr == NULL);

  client->command (client, "subst", "partkit/create_end",
		   "table", table, NULL);
  do
    {
      ptr = debconf_input ("critical", "partkit/create_end");
      end = (strtod (ptr, &endptr)) * MEGABYTE_SECTORS;
      if (ptr == endptr)
	{
	  client->command (client, "input", "high", "partkit/create_bad_num",
			   NULL);
	  client->command (client, "go", NULL);
	  endptr = NULL;
	}
      else
	client->command (client, "subst", "partkit/create_confirm",
			 "end", end, NULL);

    }
  while (endptr == NULL);

  client->command (client, "subst", "partkit/create_confirm",
		   "table", table, NULL);

  ptr = debconf_input ("critical", "partkit/create_confirm");
  if (!strstr (ptr, "true"))
    return 1;

  part = ped_partition_new (disk, part_type, NULL, start, end);
  if (!part)
    goto error_destroy_constraint;
  if (!ped_disk_add_partition (disk, part, constraint))
    goto error_destroy_part;

  ped_disk_commit (disk);
  ped_constraint_destroy (constraint);
  ped_disk_destroy (disk);
  return 1;

error_destroy_part:
  ped_partition_destroy (part);
error_destroy_constraint:
  ped_constraint_destroy (constraint);
error_close_disk:
  ped_disk_destroy (disk);
error:
  return 0;
}


int
partkit_delete (PedDevice * dev)
{
  PedDisk *disk;
  PedPartition *part;
  char *ptr;

  disk = ped_disk_new (dev);

  if (!disk)
    goto error;

  if ((ptr = partkit_get_partitions (dev)) == NULL)
    {
      partkit_error (0);
      return -1;
    }
  client->command (client, "subst", "partkit/delete_choice", "choices", ptr,
		   NULL);

  ptr = partkit_get_table (&dev);
  client->command (client, "subst", "partkit/delete_choice", "table", ptr,
		   NULL);
  free (ptr);

  ptr = debconf_input ("critical", "partkit/delete_choice");

  client->command (client, "subst", "partkit/delete_confirm",
		   "partition", ptr, NULL);

  ptr = debconf_input ("critical", "partkit/delete_confirm");
  if (!strstr (ptr, "true"))
    return 1;

  part = ped_disk_get_partition (disk, atoi (ptr));

  if (!part)
    {
      ped_exception_throw (PED_EXCEPTION_ERROR, PED_EXCEPTION_CANCEL,
			   _("Partition doesn't exist."));
      goto error_close_disk;
    }
  if (ped_partition_is_busy (part))
    {
      if (ped_exception_throw (PED_EXCEPTION_WARNING,
			       PED_EXCEPTION_IGNORE_CANCEL,
			       _("Partition is being used."))
	  != PED_EXCEPTION_IGNORE)
	goto error_close_disk;
    }

  ped_disk_delete_partition (disk, part);
  ped_disk_commit (disk);
  ped_disk_destroy (disk);
  return 1;

error_close_disk:
  ped_disk_destroy (disk);
error:
  partkit_error (0);
  return -1;

}


int
main (int argc, char *argv[])
{
  int finished = 0;
  PedDevice *dev = NULL;
  char *ptr;
  client = debconfclient_new ();
  client->command (client, "title", "Partition Editor", NULL);

  do
    {
      /* FIXME: how to get a list of available devices ? */
      ptr = /* get_device_list() */ "/dev/hdb";


      client->command (client, "subst", "partkit/select_device", "choices",
		       ptr, NULL);
      /*FIXME: how to get a default device? */
      client->command (client, "subst", "partkit/select_device", "default",
		       ptr, NULL);


      if ((dev = ped_device_get (argv[1])) == NULL)
	partkit_error (1);

      ptr = partkit_get_operations (dev);
      client->command (client, "subst", "partkit/choose_operation", "choices",
		       ptr, NULL);
      free (ptr);

      ptr = partkit_get_table (&dev);
      client->command (client, "subst", "partkit/choose_operation", "table",
		       ptr, NULL);
      free (ptr);

      ptr = debconf_input ("critical", "partkit/choose_operation");

      if (strstr (ptr, "delete"))
	partkit_delete (dev);
      else if (strstr (ptr, "create"))
	partkit_create (dev);
      else if (strstr (ptr, "exit"))
	finished = 1;
    }
  while (!finished);

  return 0;
}
