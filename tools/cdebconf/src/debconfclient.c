#include "common.h"
#include "debconfclient.h"
#include "strutl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>

static int debconfclient_handle_rsp(struct debconfclient *client)
{
    char buf[2048];
    char *v;

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

static int debconfclient_command(struct debconfclient *client, 
	const char *command, ...)
{
	va_list ap;
	char *c;
	
	fputs(command, client->out);
	va_start(ap, command);
	while ((c = va_arg(ap, char *)) != NULL) 
	{
		fputs(" ", client->out);
		fputs(c, client->out);
	}
	va_end(ap);
	fputs("\n", client->out);
	fflush(client->out); /* make sure debconf sees it to prevent deadlock */

    return debconfclient_handle_rsp(client);
}

int debconf_commandf(struct debconfclient *client, const char *cmd, ...)
{
	va_list ap;
    
    va_start(ap, cmd);
    vfprintf(client->out, cmd, ap);
    va_end(ap);
    fprintf(client->out, "\n");
    fflush(client->out);

    return debconfclient_handle_rsp(client);
}

static char *debconfclient_ret(struct debconfclient *client)
{
	return client->value;
}

struct debconfclient *debconfclient_new(void)
{
	struct debconfclient *client = NEW(struct debconfclient);
	memset(client, 0, sizeof(struct debconfclient));

	if (getenv("DEBCONF_REDIR") == NULL)
	{
		dup2(1, 3);
		dup2(2, 1);
		setenv("DEBCONF_REDIR", "1", 1);
	}

	client->command = debconfclient_command;
	client->commandf = debconf_commandf;
	client->ret = debconfclient_ret;
	client->out = fdopen(3, "a");

	return client;
}

void debconfclient_delete(struct debconfclient *client)
{
	DELETE(client->value);
	DELETE(client);
}
