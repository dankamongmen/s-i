/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: convertdb.c
 *
 * Description: Allows the user to load template/question infrom from
 *              one database to another
 *
 * $Id: debconf-convertdb.c,v 1.2 2001/01/07 05:05:12 tausq Rel $
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
#include "confmodule.h"
#include "configuration.h"
#include "frontend.h"
#include "database.h"
#include <getopt.h>
#include <unistd.h>

static struct option g_dpc_args[] = {
	{ "help", 0, NULL, 'h' },
	{ "from", 1, NULL, 'f' },
	{ "to", 1, NULL, 't' },
	{ 0, 0, 0, 0 }
};

void usage(void)
{
	printf("convertdb <-f fromdb> <-t todb> [-h]\n");
	printf("convertdb <--from fromdb> <--to todb> [--help]\n");
	printf("\tfromdb, todb - database modules to convert from/to\n");
	printf("\t-h, --help - this help message\n");
	exit(0);
}

int main(int argc, char **argv)
{
	struct configuration *config;
	struct database *db1, *db2;
	struct question *q;
	struct template *t;
	char *db1name = 0, *db2name = 0;
	void *iter;
	int c;

	while ((c = getopt_long(argc, argv, "hf:t:", g_dpc_args, NULL) > 0))
	{
		switch (c)
		{
		case 'h': usage(); break;
		case 'f': db1name = optarg; break;
		case 't': db2name = optarg; break;
		}
	}

	if (db1name == NULL || db2name == NULL)
		usage();

	/* parse the configuration info */
	config = config_new();
	if (config->read(config, "debconf.conf") == 0)
		DIE("Error reading configuration information");

	/* initialize database and frontend modules */
	setenv("DEBCONF_DB", db1name, 1);
	if ((db1 = database_new(config)) == 0)
		DIE("Cannot initialize first DebConf database");
	setenv("DEBCONF_DB", db2name, 1);
	if ((db2 = database_new(config)) == 0)
		DIE("Cannot initialize second DebConf database");

	/* load database */
	db1->load(db1);
	
	/* Iterate through all the questions and templates, and 
	 * put them into db2
	 */
	/* TODO: error checking */
	iter = 0;
	while ((t = db1->template_iterate(db1, &iter)) != NULL)
	{
		db2->template_set(db2, t);
	}
	iter = 0;
	while ((q = db1->question_iterate(db1, &iter)) != NULL)
	{
		db2->question_set(db2, q);
	}

	db2->save(db2);
	database_delete(db1);
	database_delete(db2);

	return 0;
}
