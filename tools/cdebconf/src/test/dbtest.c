#include "confmodule.h"
#include "configuration.h"
#include "frontend.h"
#include "database.h"

int main(int argc, char **argv)
{
	struct configuration *config;
	struct database *db;

	/* parse the configuration info */
	config = config_new();
	if (config->read(config, "debconf.conf") == 0)
		DIE("Error reading configuration information");

	/* initialize database and frontend modules */
	if ((db = database_new(config)) == 0)
		DIE("Cannot initialize DebConf database");

	/* load templates */
	db->load(db);

	/* shutting down .... sync the database and shutdown the modules */
	db->save(db);
	database_delete(db);

	return 0;
}
