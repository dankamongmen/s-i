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
#include "utils.h"
#include "ddetect.h"
#include "ethdetect.h"

#define ERROR 1
#define SUCCESS 0

static char *unknown_str = "unknown";
static char *ignore_str = "ignore";
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
ethdetect_install_module(char *modulename){
    char buffer[128];
    char *params=NULL;
    params = debconf_input ("high", "ethdetect/module_params");
    snprintf(buffer, sizeof(buffer), "insmod %s %s", modulename, (params ? params : " "));
    
    if ( execlog (buffer) != 0 )
	return ERROR;
    else 
	return SUCCESS;
}



static int
ethdetect_module_prompt(void){
    char *ptr;
    char *module=NULL;
    int rv=1;

    ptr = debconf_input ("high", "ethdetect/module_select");
    if (strstr(ptr, "other"))
	ptr = debconf_input ("high", "ethdetect/module_prompt");
    if(ptr) {
	module = strdup(ptr);
	if (ethdetect_install_module(module) != 0){
	    client->command (client, "input", "high", "ethdetect/error", NULL);
	    client->command (client, "go", NULL);
	    rv = ERROR;
	}
	else 
	    rv = SUCCESS;
    }
    
    if(module)
	free(module);
    return rv;
}



static int
ethdetect_detect_and_install(int passive_detection) {

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
   
   
    if (((ethernet = ethernet_detect (&bus)) == NULL) ){
	    client->command (client, "input", "high", "ethdetect/nothing_detected", NULL);
	    client->command (client, "go", NULL);
	    return ERROR;
    }
   
    for (; ethernet; ethernet = ethernet->next)
    {
	fprintf(stderr, "here\n");
	client->command (client, "subst", "ethdetect/load_module", "bus", 
		bus2str (ethernet->bus), NULL);
	client->command (client, "subst", "ethdetect/load_module", "module",
		ethernet->module, NULL);

	ptr = debconf_input ("high", "ethdetect/load_module");
	if (strstr(ptr, "true")){
	    if (ethdetect_install_module(ethernet->module) != 0){
		client->command (client, "input", "high", "ethdetect/error", NULL);
		client->command (client, "go", NULL);
		return ERROR;
	    }
	}
    }
    
    return SUCCESS;
}

int
main (int argc, char *argv[])
{
    char *ptr=NULL;
    int passive_detection=1;
   
    client = debconfclient_new ();
    client->command (client, "title", "Network Hardware Configuration", NULL);

    ptr = debconf_input ("high", "ethdetect/detection_type");
   
    if (strstr(ptr, "none")){ 
	exit(ethdetect_module_prompt());
    }
    else if (strstr(ptr, "full"))
	passive_detection = 0;
    else
	passive_detection = 1;
    
    exit(ethdetect_detect_and_install(passive_detection));
}


