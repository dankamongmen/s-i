#include "common.h"
#include "debconfclient.h"
#include "strutl.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>

static int debconfclient_command(struct debconfclient *client, 
	const char *command, ...)
{
	char buf[2048];
	va_list ap;
	char *c, *v;
	
	fputs(command, stdout);
	va_start(ap, command);
	while ((c = va_arg(ap, char *)) != NULL) 
	{
		fputs(" ", stdout);
		fputs(c, stdout);
	}
	va_end(ap);
	fputs("\n", stdout);
	fflush(stdout); /* make sure debconf sees it to prevent deadlock */

	fgets(buf, sizeof(buf), stdin);
	CHOMP(buf);
	if (strlen(buf) > 0) 
	{
		/* strip off the return code */
		strtok(buf, " \t\n");
		DELETE(client->value);
		v = strtok(NULL, "\n");
		client->value = STRDUP_NOTNULL(v);
		return atoi(buf);
	}
	else 
	{
		/* 
		 * Nothing was entered; never really happens except during
		 * debugging.
		 */
		DELETE(client->value);
		client->value = STRDUP("");
		return 0;
	}
}

static char *debconfclient_ret(struct debconfclient *client)
{
	return client->value;
}

struct debconfclient *debconfclient_new(void)
{
	struct debconfclient *client = NEW(struct debconfclient);
	memset(client, 0, sizeof(struct debconfclient));

	client->command = debconfclient_command;
	client->ret = debconfclient_ret;

	return client;
}

void debconfclient_delete(struct debconfclient *client)
{
	DELETE(client->value);
	DELETE(client);
}
