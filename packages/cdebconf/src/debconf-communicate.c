/**
 * @file debconf-communicate.c
 * @brief Configuration module interface
 */
#include "confmodule.h"
#include "configuration.h"
#include "frontend.h"
#include "database.h"

#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <locale.h>

static struct configuration *config = NULL;
static struct frontend *frontend = NULL;
static struct confmodule *confmodule = NULL;
static struct question_db *questions = NULL;
static struct template_db *templates = NULL;

static int save()
{
        return confmodule->save(confmodule);
}

static void cleanup()
{
	if (frontend != NULL)
		frontend_delete(frontend);
	if (questions != NULL)
		question_db_delete(questions);
	if (templates != NULL)
		template_db_delete(templates);
	if (config != NULL)
		config_delete(config);
}

void sighandler(int sig)
{
	save();
	if (sig != SIGUSR1) {
		cleanup();
		exit(1);
	}
}

void help(const char *exename)
{
    fprintf(stderr, "%s [package]\n", exename);
    exit(-1);
}

int main(int argc, char **argv)
{
    int code = 127;
    char buf[1024], *ret;

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGUSR1, sighandler);
	setlocale (LC_ALL, "");

	config = config_new();
	if (!config) {
	  DIE("Cannot read new config");
	}

	/* parse the configuration info */
	if (config->read(config, DEBCONFCONFIG) == 0)
		DIE("Error reading configuration information");

	/* initialize database and frontend modules */
    if ((templates = template_db_new(config, NULL)) == 0)
        DIE("Cannot initialize DebConf template database");
	if ((questions = question_db_new(config, templates, NULL)) == 0)
		DIE("Cannot initialize DebConf configuration database");
	if ((frontend = frontend_new(config, templates, questions)) == 0)
		DIE("Cannot initialize DebConf frontend");

	/* load templates and config */
	templates->methods.load(templates);
    questions->methods.load(questions);

	/* startup the confmodule; run the config script and talk to it */
	confmodule = confmodule_new(config, templates, questions, frontend);

    if (argc > 1)
        confmodule->owner = argv[1];
    else
        confmodule->owner = "unknown";

    /* start feeding user commands to debconf ... */
    while (fgets(buf, sizeof(buf), stdin))
    {
        ret = confmodule->process_command(confmodule, buf);

        /* extract the first number as the return code */
        code = atoi(ret);

        printf ("%s\n", ret);
        free(ret);
    }

	/* shutting down .... sync the database and shutdown the modules */
	save();
	cleanup();

	return code;
}

