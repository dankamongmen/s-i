/*
 * file : niccfg-manual 
 * purpose : manually configure ethernet hardware.
 * author : David Whedon <dwhedon@debian.org>
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <detect.h>
#include <debconfclient.h>
#include <string.h>
#include "utils.h"

static int niccfg_other_modules;
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


static void 
niccfg_error(){

  client->command (client, "input", "high", "niccfg/error", NULL);
  client->command (client, "go", NULL);

}

static int
niccfg_modprobe (char *modulename)
{
  char buffer[128];
  char *params = NULL;
  params = debconf_input ("high", "niccfg/module_params");
  snprintf (buffer, sizeof (buffer), "modprobe -v %s %s", modulename,
	    (params ? params : " "));

  if (execlog (buffer) != 0)
    {
	niccfg_error();
	return 1;
    }
  else
    return 0;
}



int
main (int argc, char *argv[])
{
  char *ptr = NULL;
  char *module;
  int rv;

  client = debconfclient_new ();
  client->command (client, "title", "Manual Ethernet Hardware Configuration", NULL);

  /* FIXME: automatically compile a list of nic modules? */
  client->command (client, "subst", "niccfg/module_select", "choices", 
	  "3c501, 3c503, 3c505, 3c507, 3c509, 3c515, 3x59x, 82596, eepro100, ne, ne2k-pci, tulip, via-rhine, other", NULL);


  ptr = debconf_input ("high", "niccfg/module_select");
  if (strstr (ptr, "other")){
    ptr = debconf_input ("high", "niccfg/module_prompt");
    niccfg_other_modules=1;
  }
  if (ptr)
      module = strdup (ptr);
  else{
      niccfg_error();
      return 1;
  }

  rv = niccfg_modprobe(module);
 
  return rv;
}

