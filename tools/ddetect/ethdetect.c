/*
  file : ethdetect 
  purpose : detect ethernet cards for the Debian/GNU Linux installer

  Copyright 2000-2002 David Kimdon <dwhedon@debian.org>

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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <discover.h>
#include <cdebconf/debconfclient.h>
#include <string.h>
#include <debian-installer.h>


static struct debconfclient *client;

static char *my_debconf_input(char *priority, char *template)
{
        client->command(client, "fset", template, "seen", "false", NULL);
        client->command(client, "input", priority, template, NULL);
        client->command(client, "go", NULL);
        client->command(client, "get", template, NULL);
        return client->value;
}


static int ethdetect_insmod(char *modulename) {
     char buffer[128];
     char *params = NULL;
    
     params = my_debconf_input("low", "ethdetect/module_params");
     snprintf(buffer, sizeof(buffer), "modprobe -v %s %s", modulename,
              (params ? params : " "));
     
     if (di_execlog(buffer) != 0) {
          client->command(client, "input", "critical", "ethdetect/error",
                          NULL);
          client->command(client, "go", NULL);
          return 1;
     } else {
          di_prebaseconfig_append("40ethdetect",
                                  "echo \"%s %s\" >> /target/etc/modules",
                                  modulename,
                                  (params ? params : " "));
          return 0;
     }
}


char *ethdetect_prompt(void)
{
        char *ptr;
        char *module = NULL;

        ptr = my_debconf_input("high", "ethdetect/module_select");
        if (strstr(ptr, "other"))
                ptr = my_debconf_input("high", "ethdetect/module_prompt");
        if (ptr) {
                module = strdup(ptr);
                return module;
        } else
                return NULL;
}


static struct ethernet_info *ethdetect_detect(struct cards_lst *lst)
{

        struct bus_lst bus = { 0 };
        struct ethernet_info *ethernet = (struct ethernet_info *) NULL;

        sync();
        bus.pci = pci_detect(lst);
        bus.pcmcia = pcmcia_detect(lst);

        if (((ethernet = ethernet_detect(&bus)) == NULL)) {
                client->command(client, "input", "high",
                                "ethdetect/nothing_detected", NULL);
                client->command(client, "go", NULL);
                return NULL;
        }
        return ethernet;
}


#ifndef TEST

#define ETHDETECT_PCI_LIST "/usr/share/ddetect/ethpci.lst"

int module_loaded(char *module) {
     static char *modules[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
     int i;
     for (i = 0; modules[i] != NULL && i < sizeof(modules) / sizeof(modules[0]);
	  i++) {
          if (strcmp(modules[i], module) == 0)
               return 1;
     }
     if (i < sizeof(modules) / sizeof(modules[0]))
	  modules[i] = strdup(module);
     return 0;
}

int main(int argc, char *argv[])
{
        char *ptr = NULL;
        int rv = 1;
        struct ethernet_info *ethernet = (struct ethernet_info *) NULL;
        char *module;
        struct cards_lst *lst = NULL;

        lst = init_lst(ETHDETECT_PCI_LIST, NULL, NULL);

        client = debconfclient_new();
        client->command(client, "title", "Network Hardware Configuration",
                        NULL);

        ptr = my_debconf_input("low", "ethdetect/detection_type");

        ethernet = ethdetect_detect(lst);
        if (!ethernet) {
          /* No module detected -- run manual */
          if ((module = ethdetect_prompt()) == NULL)
            return 1;
          rv = ethdetect_insmod(module);
          free(module);
        }
        for (; ethernet; ethernet = ethernet->next) {
          if (module_loaded(ethernet->module))
            continue;
          
          client->command(client, "subst",
                          "ethdetect/load_module", "bus",
                          bus2str(ethernet->bus), NULL);
          client->command(client, "subst",
                          "ethdetect/load_module", "module",
                          ethernet->module, NULL);
          
          ptr = my_debconf_input("medium", "ethdetect/load_module");
          if (strstr(ptr, "true")) {
            if (ethdetect_insmod(ethernet->module) == 0)
              rv = 0;
          }
        }
        return rv;
}

#else

#define ETHDETECT_PCI_LIST "ethpci.lst"

int main(int argc, char *argv[])
{
        struct ethernet_info *ethernet = (struct ethernet_info *) NULL;
        struct cards_lst *lst = NULL;

        lst = init_lst(ETHDETECT_PCI_LIST, NULL, NULL);

        ethernet = ethdetect_detect(lst);
        for (; ethernet; ethernet = ethernet->next) {
                fprintf(stderr, "%s\n", ethernet->module);
        };

        di_prebaseconfig_append("ethdetect",
                                "echo \"foo bar\" >> /etc/modules");
        return (0);
}

#endif
