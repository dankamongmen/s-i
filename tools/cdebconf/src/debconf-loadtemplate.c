/**
 * @file loadtemplate.c
 * @brief simple utility to load a template file into the 
 *        database
 */
#include "common.h"
#include "configuration.h"
#include "database.h"
#include "question.h"
#include "template.h"
#include "debconfclient.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

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

    setlocale(LC_ALL, "");

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
    if ((tdb = template_db_new(config, NULL)) == 0)
        DIE("Cannot initialize DebConf template database");
    if ((qdb = question_db_new(config, tdb, NULL)) == 0)
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

    if (tdb->methods.save(tdb) != DC_OK)
	exit(1);
    if (qdb->methods.save(qdb) != DC_OK)
	exit(1);
    template_db_delete(tdb);
    question_db_delete(qdb);
    config_delete(config);

    return 0;
}
