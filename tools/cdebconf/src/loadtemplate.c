#include "common.h"
#include "configuration.h"
#include "database.h"
#include "question.h"
#include "template.h"

#include <stdio.h>
#include <string.h>

void parsecmdline(struct configuration *config, int argc, char **argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "%s <owner> <template>\n", argv[0]);
		exit(-1);
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

	/* parse the configuration info */
	if (config->read(config, DEBCONFCONFIG) == 0)
		DIE("Error reading configuration information");

	/* initialize database and frontend modules */
	if ((db = database_new(config)) == 0)
		DIE("Cannot initialize DebConf database");

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
