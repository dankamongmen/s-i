#include "confmodule.h"
#include "configuration.h"
#include "frontend.h"
#include "database.h"
#include "template.h"
#include "question.h"

void loadtemplate(const char *filename, const char *owner, 
                  struct template_db *g_templates,
                  struct question_db *g_questions);

void loadtemplate(const char *filename, const char *owner, 
                  struct template_db *g_templates,
                  struct question_db *g_questions) {
	struct template *t;
	struct question *q;

	t = template_load(filename);
	while (t)
	{
		if (g_templates->methods.set(g_templates, t) != DC_OK)
			INFO(INFO_ERROR, "Cannot add template %s", t->tag);

		q = g_questions->methods.get(g_questions, t->tag);
		if (q == NULL)
		{
			q = question_new(t->tag);
			q->template = t;
		}
		question_owner_add(q, owner);
		if (g_questions->methods.set(g_questions, q) != DC_OK)
			INFO(INFO_ERROR, "Cannot add question %s", t->tag);
		question_deref(q);
		t = t->next;
	}
}

int main(int argc, char **argv)
{
	struct configuration *config;
	struct question_db *qdb;
	struct template_db *tdb;

	/* parse the configuration info */
	config = config_new();
	if (config->read(config, "debconf.conf") == 0)
		DIE("Error reading configuration information");

	/* initialize database and frontend modules */
	if ((tdb = template_db_new(config)) == 0)
		DIE("Cannot initialize DebConf template database");

	if ((qdb = question_db_new(config, tdb)) == 0)
		DIE("Cannot initialize DebConf question database");

	/* load templates */
	qdb->methods.load(qdb);
	tdb->methods.load(tdb);

        loadtemplate("test.templates","foo", tdb, qdb);

	/* shutting down .... sync the database and shutdown the modules */
	qdb->methods.save(qdb);
	tdb->methods.save(tdb);

	template_db_delete(tdb);
	question_db_delete(qdb);

	return 0;
}
