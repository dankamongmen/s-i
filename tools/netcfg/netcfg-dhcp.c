/* 
   netcfg-dhcp.c - Configure a network via dhcp for the debian-installer
   Author - David Whedon
 

*/
#include <ctype.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <debconfclient.h>
#include "utils.h"
#include "netcfg.h"

static char *interface = NULL;
static char *hostname = NULL;
static char *domain = NULL;
static u_int32_t ipaddress = 0;
static u_int32_t nameserver_array[4] = { 0 };
static struct debconfclient *client;


static char *dhcp_hostname = NULL;

char *
debconf_input (char *priority, char *template)
{
  client->command (client, "fset", template, "seen", "false", NULL);
  client->command (client, "input", priority, template, NULL);
  client->command (client, "go", NULL);
  client->command (client, "get", template, NULL);
  return client->value;
}

static void
netcfg_get_dhcp ()
{
  if (dhcp_hostname)
    free (dhcp_hostname);

  client->command (client, "input", "high", "netcfg/dhcp_hostname", NULL);
  client->command (client, "go", NULL);
  client->command (client, "get", "netcfg/dhcp_hostname", NULL);

  if (client->value)
      dhcp_hostname = strdup (client->value);
  
  client->command (client, "subst", "netcfg/confirm_dhcp",
	  "dhcp_hostname",
	  (dhcp_hostname ? dhcp_hostname : "<none>"), NULL);
}


static void
netcfg_write_dhcp ()
{

  FILE *fp;

  if ((fp = file_open (INTERFACES_FILE)))
    {
      fprintf (fp,
	       "\n# This entry was created during the Debian installation\n");
      fprintf (fp, "iface %s inet dhcp\n", interface);
      fclose (fp);
    }
#if defined PUMP
/* nothing to do */

#elif defined DHCLIENT
  netcfg_mkdir (DHCLIENT_DIR);
  
  if (dhcp_hostname)
    if ((fp = file_open (DHCLIENT_FILE)))
      {
	fprintf (fp,
		 "\n# dhclient configuration: created during the Debian installation\n\
interface \"%s\" {\nsend host-name \"%s\";\n}\n",
		 interface, dhcp_hostname);
	fclose (fp);
      }

#else
#error "Must specify a dhcp client"
#endif
}


static void
netcfg_activate_dhcp ()
{
  char buf[128];
  char *ptr;
  execlog ("/sbin/ifconfig lo 127.0.0.1");
  ptr = buf;

#if defined PUMP

  ptr += snprintf (buf, sizeof (buf), "/sbin/pump -i %s", interface);
  if (dhcp_hostname)
    ptr +=
      snprintf (ptr, sizeof (buf) - (ptr - buf), " -h %s", dhcp_hostname);

#elif defined DHCLIENT

  ptr += snprintf (buf, sizeof (buf), "/sbin/dhclient-2.2.x %s", interface);

#else

#error "Must specify a dhcp client"
#endif

  if (execlog (buf))
    netcfg_die (client);

}

int
main (int argc, char *argv[])
{
  char *ptr;
  char *nameservers=NULL;
  int finished = 0;
  client = debconfclient_new ();
  client->command (client, "title", "DHCP Network Configuration", NULL);

  do
    {
      netcfg_get_common (client, &interface, &hostname, &domain, &nameservers);
           
      client->command (client, "subst", "netcfg/confirm_dhcp", "interface",
		       interface, NULL);
      
      client->command (client, "subst", "netcfg/confirm_dhcp", "hostname",
		       hostname, NULL);
      
      client->command (client, "subst", "netcfg/confirm_dhcp", "domain",
		       (domain ? domain : "<none>"), NULL);
      
      netcfg_nameservers_to_array(nameservers, nameserver_array);
      
      client->command (client, "subst", "netcfg/confirm_dhcp",
		       "nameservers", (nameservers ? nameservers : "<none>"), NULL);
      netcfg_get_dhcp ();
     
     
      ptr = debconf_input ("medium", "netcfg/confirm_dhcp");

      if (strstr (ptr, "true"))
	finished = 1;
    }
  while (!finished);

  netcfg_write_dhcp ();
  netcfg_write_common (ipaddress, domain, hostname, nameserver_array);

  debconf_input ("medium", "netcfg/do_dhcp");
  netcfg_activate_dhcp ();

  return 0;
}
