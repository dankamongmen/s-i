/* 
   parted-tiny.c - Configure the network for the debian-installer
   Author - David Whedon
 

*/
#include <stdlib.h>
#include <debconfclient.h>

static struct debconfclient *client;
char *current_drive=NULL;

char *
debconf_input (char *priority, char *template)
{
  client->command (client, "fset", template, "seen", "false", NULL);
  client->command (client, "input", priority, template, NULL);
  client->command (client, "go", NULL);
  client->command (client, "get", template, NULL);
  return client->value;
}


void
parted_get_drive(){
    /* FIXME: obviously this needs to be dynamically determined, perhaps we can
     * even figure out a reasonable default. */
    const char *drives="/dev/hda, /dev/hdb, /dev/hdc";
    
    client->command (client, "subst", "parted-tiny/choose_drive", drives, NULL);
    
    current_drive = debconf_input ("high", "parted-tiny/choose_drive");

}

void
parted_partition_drive(char *drive){


}


int
main (int argc, char *argv[])
{
  int finished = 0;
  client = debconfclient_new ();

  client->command (client, "title", "Disk Partitioning", NULL);


  do
    {
	parted_get_drive();
	parted_partition_drive(current_drive);
	finished =1;
    }
  while (!finished);


  return 0;
}

