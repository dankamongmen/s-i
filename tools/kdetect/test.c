#include "libkdetect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	char *device_list = NULL;
	char *default_device = NULL;
	
	device_list = (char *) malloc(512);
	device_list = get_device_list();
	printf("device list is [%s]\n",device_list);

	default_device = (char *) malloc(strlen(device_list));
	default_device = strrchr(device_list, ' ');
	default_device = get_default_device();
	

	return 1;
}
