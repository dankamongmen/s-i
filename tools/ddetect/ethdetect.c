/*
 * file : ddetect 
 * purpose : detect ethernet cards for the Debian/GNU Linux installer
 * author : David Whedon <dwhedon@gordian.com>
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <discover.h>
#include <cdebconf/debconfclient.h>
#include <string.h>
#include <debian-installer.h>
#include "ddetect.h"
#include "ethdetect.h"

#define ERROR 1
#define SUCCESS 0

static struct debconfclient *client;

static char *
debconf_input (char *priority, char *template)
{
  client->command (client, "fset", template, "seen", "false", NULL);
  client->command (client, "input", priority, template, NULL);
  client->command (client, "go", NULL);
  client->command (client, "get", template, NULL);
  return client->value;
}


static int
ethdetect_insmod (char *modulename)
{
  char buffer[128];
  char *params = NULL;
  params = debconf_input ("high", "ethdetect/module_params");
  snprintf (buffer, sizeof (buffer), "modprobe -v %s %s", modulename,
	    (params ? params : " "));

  if (di_execlog (buffer) != 0)
    {
      client->command (client, "input", "high", "ethdetect/error", NULL);
      client->command (client, "go", NULL);
      return 1;
    }
  else
    return 0;
}



char *
ethdetect_prompt (void)
{
  char *ptr;
  char *module = NULL;

  ptr = debconf_input ("high", "ethdetect/module_select");
  if (strstr (ptr, "other"))
    ptr = debconf_input ("high", "ethdetect/module_prompt");
  if (ptr)
    {
      module = strdup (ptr);
      return module;
    }
  else
    return NULL;
}



static struct ethernet_info *
ethdetect_detect (int passive_detection)
{

  struct bus_lst bus = { 0 };
  struct ethernet_info *ethernet = (struct ethernet_info *) NULL;
  int i = 0;

  while (lst[i].vendor)
    {
      lst[i].next = &lst[++i];
    }
  lst[i - 1].next = NULL;


  sync ();
  bus.pci = pci_detect (lst);
  if (!passive_detection)
    bus.isa = isa_detect (lst);

  bus.pcmcia = pcmcia_detect (lst);


  if (((ethernet = ethernet_detect (&bus)) == NULL))
    {
      client->command (client, "input", "high", "ethdetect/nothing_detected",
		       NULL);
      client->command (client, "go", NULL);
      return NULL;
    }
  return ethernet;
}


#ifndef TEST
int
main (int argc, char *argv[])
{
  char *ptr = NULL;
  int passive_detection = 1, rv = 1;
  struct ethernet_info *ethernet = (struct ethernet_info *) NULL;
  char *module;


  client = debconfclient_new ();
  client->command (client, "title", "Network Hardware Configuration", NULL);

  ptr = debconf_input ("high", "ethdetect/detection_type");

  if (strstr (ptr, "none"))
    {
      if ((module = ethdetect_prompt ()) == NULL)
	return 1;
      rv = ethdetect_insmod (module);
      free (module);
    }
  else
    {
      if (strstr (ptr, "full"))
	passive_detection = 0;
      else
	passive_detection = 1;

      ethernet = ethdetect_detect (passive_detection);
      for (; ethernet; ethernet = ethernet->next)
	{
	  client->command (client, "subst", "ethdetect/load_module", "bus",
			   bus2str (ethernet->bus), NULL);
	  client->command (client, "subst", "ethdetect/load_module", "module",
			   ethernet->module, NULL);

	  ptr = debconf_input ("high", "ethdetect/load_module");
	  if (strstr (ptr, "true"))
	    {
	      if (ethdetect_insmod (ethernet->module) == 0)
		rv = 0;
	    }
	}
    }
  return rv;
}

#else

int
main (int argc, char *argv[])
{
  struct ethernet_info *ethernet = (struct ethernet_info *) NULL;
  
  ethernet = ethdetect_detect(0);
  for (; ethernet; ethernet = ethernet->next) {
    fprintf(stderr, "%s\n", ethernet->module);
  };
  return (0);
}

#endif
