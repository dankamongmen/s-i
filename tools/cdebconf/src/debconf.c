/**
 * @file debconf.c
 * @brief Configuration module interface
 */
#include "confmodule.h"
#include "configuration.h"
#include "question.h"
#include "frontend.h"
#include "database.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <locale.h>

static struct configuration *config = NULL;
static struct frontend *frontend = NULL;
static struct confmodule *confmodule = NULL;
static struct question_db *questions = NULL;
static struct template_db *templates = NULL;
static char *owner;

static struct option options[] = {
    { "owner", 1, 0, 'o' },
    { "frontend", 1, 0, 'f' },
    { "priority", 1, 0, 'p' },
    { 0, 0, 0, 0 },
};

static void save()
{
	if (confmodule != NULL)
		confmodule->update_seen_questions(confmodule, STACK_SEEN_SAVE);
	if (questions != NULL)
		questions->methods.save(questions);
	if (templates != NULL)
		templates->methods.save(templates);
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
	struct question *q = NULL;

	save();
	if (sig == SIGUSR1)
	{
		if (questions != NULL)
		{
			q = questions->methods.get(questions, "debconf/language");
			if (q != NULL)
				setenv("LANGUAGE", question_getvalue(q, NULL), 1);
		}
		return;
	}
	cleanup();
	exit(1);
}

void help(const char *exename)
{
    fprintf(stderr, "%s [-ffrontend] [-ppriority] [-oowner] <config script>\n", exename);
    fprintf(stderr, "%s [--frontend=frontend] [--priority=priority] [--owner=owner] <config script>\n", exename);
    exit(-1);
}

void parsecmdline(struct configuration *config, int argc, char **argv)
{
    int c;

    while ((c = getopt_long(argc, argv, "o:p:f:", options, NULL)) > 0)
    {
        switch (c)
        {
            case 'o':
                owner = optarg;
                break;
            case 'p':
                break;
            case 'f':
		        config->set(config, "frontend::default::driver", optarg);
                break;
            default:
                break;
        }
    }

    if (optind >= argc)
    {
        help(argv[0]);
    }
}

int main(int argc, char **argv)
{
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGUSR1, sighandler);
	setlocale (LC_ALL, "");

	config = config_new();
	if (!config) {
	  DIE("Cannot read new config");
	}
	parsecmdline(config, argc, argv);

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
	/* set title */
	{
		char buf[100], pkg[100];
		char *slash = strrchr(argv[optind], '/');
		if (slash == NULL) slash = argv[optind]; else slash++;
		snprintf(pkg, sizeof(pkg), "%s", slash);
		if (strlen(pkg) >= 7 
			&& strcmp(pkg + strlen(pkg) - 7, ".config") == 0)
		{
			pkg[strlen(pkg) - 7] = '\0';
		}
		snprintf(buf, sizeof(buf), "Configuring %s", pkg);
		frontend->methods.set_title(frontend, buf);
	}

	/* load templates and config */
	templates->methods.load(templates);
        questions->methods.load(questions);

	/* startup the confmodule; run the config script and talk to it */
	confmodule = confmodule_new(config, templates, questions, frontend);
        if (owner == NULL)
          owner = "unknown";
        confmodule->owner = owner;
	confmodule->run(confmodule, argc - optind + 1, argv + optind - 1);
	confmodule->communicate(confmodule);

	/* shutting down .... sync the database and shutdown the modules */
	save();
	cleanup();

	return confmodule->exitcode;
}
