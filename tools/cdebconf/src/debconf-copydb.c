/**
 * @file debconf-copydb.c
 * @brief Allows the user to load template/question from
 *        one database to another
 */
#include "confmodule.h"
#include "configuration.h"
#include "frontend.h"
#include "database.h"
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
    printf("%s [-h|--help] source-db dest-db\n", exename);
    printf("\tsource-db, dest-db - config databases to copy from/to\n");
    printf("\t-h, --help - this help message\n");
    exit(0);
}

int main(int argc, char **argv)
{
    struct configuration *config;
    struct template_db *tdb;
    struct question_db *db1, *db2;
    struct question *q;
#if 0
    struct template *t;
#endif
    char *db1name = 0, *db2name = 0;
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

    if (optind + 2 > argc)
        usage(argv[0]);

    db1name = argv[optind];
    db2name = argv[optind+1];

    if (db1name == NULL || db2name == NULL)
        usage(argv[0]);

    /* parse the configuration info */
    config = config_new();
    if (config->read(config, DEBCONFCONFIG) == 0)
        DIE("Error reading configuration information");

    /* initialize database modules */
    if ((tdb = template_db_new(config, NULL)) == 0)
        DIE("Cannot initialize DebConf template database");
    setenv("DEBCONF_CONFIG", db1name, 1);
    if ((db1 = question_db_new(config, tdb, NULL)) == 0)
        DIE("Cannot initialize first DebConf database");
    setenv("DEBCONF_CONFIG", db2name, 1);
    if ((db2 = question_db_new(config, tdb, NULL)) == 0)
        DIE("Cannot initialize second DebConf database");

    /* load database */
    tdb->methods.load(tdb);
    db1->methods.load(db1);
    db2->methods.load(db2);
    
    /* 
     * Iterate through all the questions and put them into db2
     */

    /* TODO: error checking */
    iter = 0;
    while ((q = db1->methods.iterate(db1, &iter)) != NULL)
    {
        db2->methods.set(db2, q);
        question_deref(q);
    }

    db2->methods.save(db2);
    question_db_delete(db1);
    question_db_delete(db2);
    template_db_delete(tdb);

    return 0;
}

