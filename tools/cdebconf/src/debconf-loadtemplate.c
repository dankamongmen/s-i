/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: loadtemplate.c
 *
 * Description: simple utility to load a template file into the 
 *              database
 *
 * $Id: debconf-loadtemplate.c,v 1.3 2002/05/27 14:25:09 tfheen Rel $
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
#include "configuration.h"
#include "database.h"
#include "question.h"
#include "template.h"
#include "debconfclient.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void parsecmdline(struct configuration *config, int argc, char **argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "%s <owner> <template>\n", argv[0]);
		exit(-1);
	}
}

void add_questions_debconf(int argc, char **argv)
{
        int i = 2;
        struct debconfclient *client;
        client = debconfclient_new ();
        
	while (i <= argc)
	{
                if (argv[i])
                        client->command (client, "X_LOADTEMPLATEFILE", 
                                         argv[i], NULL);
                i++;
	}
}

int main(int argc, char **argv)
{
	struct configuration *config = NULL;
	struct database *db = NULL;
	struct template *t = NULL;
	struct question *q = NULL;
	int i = 2;

	config = config_new();
	parsecmdline(config, argc, argv);

        if (getenv("DEBIAN_HAS_FRONTEND") != NULL)
          {
                  /* Use debconfclient instead.  This is a kinda hack
                   * for debian-installer, but not too ugly.  
                   * -- tfheen, 2002-05-23
                   */
                  add_questions_debconf(argc, argv);
                  exit(0);
          }
	/* parse the configuration info */
	if (config->read(config, DEBCONFCONFIG) == 0)
		DIE("Error reading configuration information");

	/* initialize database and frontend modules */
	if ((db = database_new(config)) == 0)
		DIE("Cannot initialize DebConf database");

        db->load(db);

	while (i <= argc)
	{
		t = template_load(argv[i++]);
		while (t)
		{
			if (db->template_set(db, t) != DC_OK)
				INFO(INFO_ERROR, "Cannot add template %s", t->tag);

			q = db->question_get(db, t->tag);
			if (q == NULL)
			{
				q = question_new(t->tag);
				q->template = t;
			}
			question_owner_add(q, argv[1]);
			if (db->question_set(db, q) != DC_OK)
				INFO(INFO_ERROR, "Cannot add template %s", t->tag);
			question_deref(q);
			t = t->next;
		}
	}

	db->save(db);
	database_delete(db);
	config_delete(config);

	return 0;
}
