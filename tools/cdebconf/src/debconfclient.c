/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: debconfclient.c
 *
 * Description: debconf client support interface
 *
 * $Id: debconfclient.c,v 1.4 2001/01/07 05:05:12 tausq Rel $
 *
 * cdebconf is (c) 2000-2001 Randolph Chung and others under the following
 * license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ***********************************************************************/
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

/*
 * Function: debconfclient_command
 * Input: struct debconfclient *client - client object
 *        const char *command, ... - null terminated command string
 * Output: return code from confmodule
 * Description: sends a command to the confmodule
 * Assumptions: command list is NULL terminated
 */
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
		client->value = STRDUP(v);
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

/*
 * Function: debconfclient_ret
 * Input: struct debconfclient *client - client object
 * Output: char * - return value
 * Description: simple accessor method for the return value
 * Assumptions: client != NULL
 */
static char *debconfclient_ret(struct debconfclient *client)
{
	return client->value;
}

/*
 * Function: debconfclient_new
 * Input: none
 * Output: struct debconfclient * - newly created debconfclient object
 * Description: creates  a debconfclient object
 * Assumptions: none
 */
struct debconfclient *debconfclient_new(void)
{
	struct debconfclient *client = NEW(struct debconfclient);
	memset(client, 0, sizeof(struct debconfclient));

	client->command = debconfclient_command;
	client->ret = debconfclient_ret;

	return client;
}

/*
 * Function: debconfclient_delete
 * Input: struct debconfclient *client - client object to destroy
 * Output: none
 * Description: destroys the debconfclient object 
 * Assumptions: client != NULL
 */
void debconfclient_delete(struct debconfclient *client)
{
	DELETE(client->value);
	DELETE(client);
}
