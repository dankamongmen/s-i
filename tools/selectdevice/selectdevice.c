#include "libkdetect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "//usr/local/include/cdebconf/debconfclient.h"

int main(int argc, char **argv)
{
	struct debconfclient *client;
	char *device_list = NULL;
	char *default_device = NULL;
	
	device_list = (char *) malloc(512);
	device_list = get_device_list();
	printf("device list is [%s]\n",device_list);

	default_device = (char *) malloc(strlen(device_list));
	default_device = strrchr(device_list, ' ');
//	default_device = get_default_device();
	
	client = debconfclient_new ();
	client->command (client, "title", "Select device to partition", NULL);
	client->command (client, "subst", "kdetect/select_device", "device_list", device_list, NULL);
	client->command (client, "subst", "kdetect/select_device", "default_device", default_device, NULL);
	client->command (client, "fset", "kdetect/select_device", "seen", "false", NULL);
	client->command (client, "input", "high", "kdetect/select_device", NULL);
	client->command (client, "go", NULL);
	client->command (client, "get", "kdetect/select_device", NULL);

	return 1;
}
