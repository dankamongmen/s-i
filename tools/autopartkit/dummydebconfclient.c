#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "dummydebconfclient.h"

struct {
  const char *valname;
  const char *value;
} defaults[] = {
  {"autopartkit/device_name", "testdevice"},
  {"autopartkit/confirm", "true"},
  {"debian-installer/profile", "Server, LTSP-server"},
  {NULL, NULL}
};

static void
my_debconfclient_command(struct debconfclient *client,
			 const char *operation, ...)
{
  va_list ap;
  va_start(ap, operation);
  if (0 == strcmp(operation, "GET")) {
    char *valname;
    int i;
    valname = va_arg(ap, char *);
    client->value = NULL;
    for (i=0; defaults[i].valname; i++) {
      if (0 == strcmp(valname, defaults[i].valname)) {
	client->value = defaults[i].value;
      }
    }
    printf("debconf get valname: '%s' = '%s'\n",
	   valname ? valname : "[null]",
	   client->value ? client->value : "[null]");
  }
  va_end(ap);
}

struct debconfclient *
my_debconfclient_new(void)
{
  struct debconfclient *retval = malloc(sizeof(*retval));
  if (retval) {
    retval->value = NULL;
    retval->command = my_debconfclient_command;
  }
  return retval;
}

void
my_debconfclient_delete(struct debconfclient *client)
{
  free(client);
}
