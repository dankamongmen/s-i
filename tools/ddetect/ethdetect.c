/*
 * file : ddetect 
 * purpose : detect ethernet cards for the Debian/GNU Linux installer
 * author : David Whedon <dwhedon@gordian.com>
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <detect.h>
#include <debconfclient.h>
#include <string.h>

#include "ddetect.h"
#include "ethdetect.h"

static char buffer[128];
static char *unknown_str = "unknown";
static char *ignore_str = "ignore";
static struct debconfclient *client;


static char * debconf_input (char *priority, char *template);
static int install_module(char *modulename);
static int detect_and_install(int passive_detection);

int
main (int argc, char *argv[])
{
    char *ptr=NULL;
    int passive_detection;
    
    client = debconfclient_new ();

    ptr = debconf_input ("high", "ddetect/detection_type");

    if (strstr(ptr, "none")) {
	ptr = debconf_input ("high", "ethdetect/module_select");
	if (strstr(ptr, "other"))
	    ptr = debconf_input ("high", "ethdetect/module_prompt");
	if(ptr)
	    if (install_module(ptr) != 0){
		client->command (client, "input", "high", "ethdetect/error", NULL);
		client->command (client, "go", NULL);
		exit(1);
	    }
	exit(0);
    }

    if (strstr(ptr, "full"))
	passive_detection = 0;
    else
	passive_detection = 1;
	
    return detect_and_install(passive_detection);

}


static char *
debconf_input (char *priority, char *template)
{
  client->command (client, "input", priority, template, NULL);
  client->command (client, "go", NULL);
  client->command (client, "get", template, NULL);
  return client->value;
}


static int
install_module(char *modulename){
    char *params=NULL;
    params = debconf_input ("high", "ethdetect/module_params");
    snprintf(buffer, sizeof(buffer), "insmod %s %s", modulename, (params ? params : " "));
    return system (buffer);
}

int
detect_and_install(int passive_detection) {

    struct bus_lst bus = { 0 };
    struct ethernet_info *ethernet = (struct ethernet_info *) NULL;
    int i, j, num_cards;
    struct cards_lst *lst = NULL;
    char *ptr;
   
    num_cards = 0;
    for (i = 0; pci_modules[i].name; i++)
	for (j = 0; pci_modules[i].id[j].long_id; j++)
	    num_cards++;
    
    if (!passive_detection)
    {
       	for (i = 0; isa_modules[i].name; i++)
    	    for (j = 0; isa_modules[i].id[j].dev_id; j++)
      		num_cards++;
    }
    
    lst = malloc (sizeof (struct cards_lst) * num_cards);
    
    num_cards = 0;
    
    /* initialize list of pci cards */
    for (i = 0; pci_modules[i].name; i++)
	for (j = 0; pci_modules[i].id[j].long_id; j++)
	{
	    lst[num_cards].vendor = unknown_str;
	    lst[num_cards].model = unknown_str;
	    lst[num_cards].modulename = pci_modules[i].name;
	    lst[num_cards].bus = PCI;
	    lst[num_cards].dev_id = ignore_str;
	    lst[num_cards].long_id = pci_modules[i].id[j].long_id;
	    lst[num_cards].type = ETHERNETCARD;
	    lst[num_cards].options = 0;
	    lst[num_cards].next = &lst[++num_cards];
	}
    
    
    if (!passive_detection)
    {
       	/* initialize list of isa cards */
	for (i = 0; isa_modules[i].name; i++)
	    for (j = 0; isa_modules[i].id[j].dev_id; j++)
	    {
		lst[num_cards].vendor = unknown_str;
		lst[num_cards].model = unknown_str;
		lst[num_cards].modulename = isa_modules[i].name;
		lst[num_cards].bus = ISA;
		lst[num_cards].dev_id = isa_modules[i].id[j].dev_id;
		lst[num_cards].long_id = 0;
		lst[num_cards].type = ETHERNETCARD;
		lst[num_cards].options = 0;
		lst[num_cards].next = &lst[++num_cards];
	    }
    }
    
    lst[num_cards - 1].next = NULL;
    
    
    sync ();
    bus.pci = pci_detect (lst);
    if (!passive_detection)
	bus.isa = isa_detect (lst);
    
    bus.pcmcia = pcmcia_detect (lst);
    
    for (; ethernet; ethernet = ethernet->next)
    {
	client->command (client, "subst", "ethdetect/load_module", "bus", 
		bus2str (ethernet->bus), NULL);
	client->command (client, "subst", "ethdetect/load_module", "module",
		ethernet->module, NULL);

	ptr = debconf_input ("high", "ethdetect/load_module");
	if (strstr(ptr, "true")){
	    if (install_module(ethernet->module) != 0){
		client->command (client, "input", "high", "ethdetect/error", NULL);
		client->command (client, "go", NULL);
	    }
	}
    }
    
    return 0;
}
