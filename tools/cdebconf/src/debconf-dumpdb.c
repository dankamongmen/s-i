/**
 * @file debconf-dumpdb.c
 * @brief Dump a database
 *
 * This code was found on debian-boot@lists.debian.org 2003-07-30,
 * posted by Tollef Fog Heen.
 *
 */
#include "confmodule.h"
#include "configuration.h"
#include "frontend.h"
#include "database.h"
#include "template.h"
#include "question.h"

#include <getopt.h>
#include <unistd.h>
#include <locale.h>

static struct option g_dpc_args[] = {
    { "help", 0, NULL, 'h' },
    { 0, 0, 0, 0 }
};

void usage(const char *exename)
{
    printf("%s [-h|--help] [source-db]\n", exename);
    printf("\tsource-db -  config database to dump\n");
    printf("\t-h, --help - this help message\n");
    exit(0);
}

int main(int argc, char **argv)
{
    struct configuration *config;
    struct template_db *tdb;
    struct question_db *qdb;
    struct question *q;
    char *dbname = 0;
    void *iter;
    int c;

    setlocale(LC_ALL, "");
    
    while ((c = getopt_long(argc, argv, "h", g_dpc_args, NULL) > 0))
    {
        switch (c)
        {
        case 'h': usage(argv[0]); break;
        }
    }

    if (optind + 1 > argc)
        usage(argv[0]);

    dbname = argv[optind];

    if (dbname == NULL)
        usage(argv[0]);

    /* parse the configuration info */
    config = config_new();
    if (config->read(config, DEBCONFCONFIG) == 0)
        DIE("Error reading configuration information");

    /* initialize database modules */
    if ((tdb = template_db_new(config, NULL)) == 0)
        DIE("Cannot initialize debconf template database");
    setenv("DEBCONF_CONFIG", dbname, 1);
    if ((qdb = question_db_new(config, tdb, NULL)) == 0)
        DIE("Cannot initialize debconf database");

    /* load database */
    tdb->methods.load(tdb);
    qdb->methods.load(qdb);
    
    /* 
     * Iterate through all the questions and print them out
     */

    /* TODO: error checking */
    iter = 0;
    while ((q = qdb->methods.iterate(qdb, &iter)) != NULL)
    {
        printf("%s %s %s\n", q->tag, q->template->type, q->value);
        question_deref(q);
    }

    question_db_delete(qdb);
    template_db_delete(tdb);

    return 0;
}
