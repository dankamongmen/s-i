#include "confmodule.h"
#include "configuration.h"
#include "frontend.h"
#include "database.h"

#include <signal.h>
#include <string.h>

static struct configuration *config = NULL;
static struct frontend *frontend = NULL;
static struct confmodule *confmodule = NULL;
static struct database *db = NULL;

void cleanup()
{
	if (db != NULL)
		db->save(db);
	if (frontend != NULL)
		frontend_delete(frontend);
	if (db != NULL)
		database_delete(db);
	if (config != NULL)
		config_delete(config);
}

void sighandler(int sig)
{
	cleanup();
}

void parsecmdline(struct configuration *config, int argc, char **argv)
{
	if (argc <= 1)
	{
		fprintf(stderr, "%s <config script>\n", argv[0]);
		exit(-1);
	}
}

int main(int argc, char **argv)
{
	signal(SIGINT, sighandler);

	config = config_new();
	if (!config) {
	  DIE("Cannot read new config");
	}
	parsecmdline(config, argc, argv);

	/* parse the configuration info */
	if (config->read(config, DEBCONFCONFIG) == 0)
		DIE("Error reading configuration information");

	/* initialize database and frontend modules */
	if ((db = database_new(config)) == 0)
		DIE("Cannot initialize DebConf database");
	if ((frontend = frontend_new(config, db)) == 0)
		DIE("Cannot initialize DebConf frontend");
	/* set title */
	{
		char buf[100], pkg[100];
		char *slash = strrchr(argv[1], '/');
		if (slash == NULL) slash = argv[1]; else slash++;
		snprintf(pkg, sizeof(pkg), "%s", slash);
		if (strlen(pkg) >= 7 
			&& strcmp(pkg + strlen(pkg) - 7, ".config") == 0)
		{
			pkg[strlen(pkg) - 7] = '\0';
		}
		snprintf(buf, sizeof(buf), "Configuring %s", pkg);
		frontend->set_title(frontend, buf);
	}

	/* load templates */
	db->load(db);

	/* startup the confmodule; run the config script and talk to it */
	confmodule = confmodule_new(config, db, frontend);
	confmodule->run(confmodule, argc, argv);
	confmodule->communicate(confmodule);

	/* shutting down .... sync the database and shutdown the modules */
	cleanup();

	return confmodule->exitcode;
}
