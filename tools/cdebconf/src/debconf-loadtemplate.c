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
 * $Id: debconf-loadtemplate.c,v 1.6 2002/08/07 16:38:19 tfheen Exp $
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
    int i;
    struct debconfclient *client;
    client = debconfclient_new ();
        
    for (i = 2; i <= argc; i++)
    {
        if (argv[i])
            client->command (client, "X_LOADTEMPLATEFILE", 
                                         argv[i], NULL);
    }
}

int main(int argc, char **argv)
{
    struct configuration *config = NULL;
    struct question_db *qdb = NULL;
    struct template_db *tdb = NULL;
    struct template *t = NULL;
    struct question *q = NULL;
    int i = 2;

    config = config_new();
    parsecmdline(config, argc, argv);

    /* If debconf is already running, use debconfclient to load
     * the templates;
     * This is a hack until we introduce a standard debconf
     * primitive for doing this.
     */
    if (getenv("DEBIAN_HAS_FRONTEND") != NULL)
    {
         add_questions_debconf(argc, argv);
         exit(0);
    }

    /* parse the configuration info */
    if (config->read(config, DEBCONFCONFIG) == 0)
        DIE("Error reading configuration information");

    /* initialize database modules */
    if ((tdb = template_db_new(config)) == 0)
        DIE("Cannot initialize DebConf template database");
    if ((qdb = question_db_new(config, tdb)) == 0)
        DIE("Cannot initialize DebConf config database");

    tdb->methods.load(tdb);
    qdb->methods.load(qdb);

    while (i < argc)
    {
        t = template_load(argv[i++]);
        while (t)
        {
            if (tdb->methods.set(tdb, t) != DC_OK)
                INFO(INFO_ERROR, "Cannot add template %s", t->tag);

            q = qdb->methods.get(qdb, t->tag);
            if (q == NULL)
            {
                q = question_new(t->tag);
                q->template = t;
                template_ref(t);
            }
            question_owner_add(q, argv[1]);
            if (qdb->methods.set(qdb, q) != DC_OK)
                INFO(INFO_ERROR, "Cannot add config %s", t->tag);
            question_deref(q);
            t = t->next;
        }
    }

    tdb->methods.save(tdb);
    qdb->methods.save(qdb);
    template_db_delete(tdb);
    question_db_delete(qdb);
    config_delete(config);

    return 0;
}
