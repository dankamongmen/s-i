#include "common.h"
#include "configuration.h"
#include "database.h"
#include "textdb.h"
#include "template.h"
#include "question.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

static void translate_tag_name(char *buf)
{
	/* remove / from the tag name so that we can use it as a filename */
	char *t = buf;
	for (; *t != 0; t++)
		if (*t == '/') *t = ':';  /* : is illegal in templates etc */
}

static char *template_filename(struct configuration *cfg, const char *tag)
{
	static char filename[1024];
	char tagname[1024];
	filename[0] = 0;

	strncpy(tagname, tag, sizeof(tagname));
	translate_tag_name(tagname);

	snprintf(filename, sizeof(filename), "%s/%s",
		cfg->get(cfg, "database::driver::textdb::templatepath", 
		TEXTDB_TEMPLATE_PATH), tagname);

	return filename;
}

static char *question_filename(struct configuration *cfg, const char *tag)
{
	static char filename[1024];
	char tagname[1024];
	filename[0] = 0;

	strncpy(tagname, tag, sizeof(tagname));
	translate_tag_name(tagname);

	snprintf(filename, sizeof(filename), "%s/%s",
		cfg->get(cfg, "database::driver::textdb::questionpath", 
		TEXTDB_QUESTION_PATH), tagname);

	return filename;
}

static struct template *textdb_lookup_cached_template(
	const struct database *db, const char *tag)
{
	struct db_cache *dbdata = db->data;
	struct template *result;
	for (result = dbdata->templates; result; result = result->next)
	{
		if (strcmp(result->tag, tag) == 0) break;
	}
	return result;
}

static void textdb_remove_cached_template(struct database *db,
	const char *tag)
{
	struct db_cache *dbdata = db->data;
	struct template **result;
	for (result = &dbdata->templates; *result; result = &(*result)->next)
	{
		if (strcmp((*result)->tag, tag) == 0)
		{
			*result = (*result)->next;
			break;
		}
	}
}

static struct question *textdb_lookup_cached_question(
	const struct database *db, const char *tag)
{
	struct db_cache *dbdata = db->data;
	struct question *result;
	for (result = dbdata->questions; result; result = result->next) 
	{
		if (strcmp(result->tag, tag) == 0) break;
	}
	return result;
}

static int textdb_initialize(struct database *db, struct configuration *cfg)
{
	struct db_cache *dbdata;
	dbdata = malloc(sizeof(struct db_cache));

	if (dbdata == NULL)
		return DC_NOTOK;

	dbdata->questions = NULL;
	dbdata->templates = NULL;
	db->data = dbdata;

	return DC_OK;
}

static int textdb_load(struct database *db)
{
	return DC_OK;
}

static int textdb_save(struct database *db)
{
	return DC_OK;
}

static int textdb_template_add(struct database *db, struct template *t)
{
	FILE *outf;
	char *filename;

	if (t->tag == NULL) return DC_NOTOK;
	filename = template_filename(db->config, t->tag);
	
	if ((outf = fopen(filename, "w")) == NULL)
		return DC_NOTOK;

	fprintf(outf, "template {\n");

	fprintf(outf, "\tname \"%s\";\n", t->tag);
	fprintf(outf, "\ttype \"%s\";\n", t->type);
	if (t->defaultval != NULL)
		fprintf(outf, "\tdefault \"%s\";\n", t->defaultval);
	if (t->choices != NULL)
		fprintf(outf, "\tchoices \"%s\";\n", t->choices);
	if (t->description != NULL)
		fprintf(outf, "\tdescription \"%s\";\n", t->description);
	if (t->extended_description != NULL)
		fprintf(outf, "\textended_description \"%s\";\n", 
			t->extended_description);

	fprintf(outf, "}\n");
	fclose(outf);
	
	return DC_OK;
}

static struct template *textdb_template_get_real(struct database *db, 
	const char *ltag)
{
	struct configuration *rec;
	struct template *t;
	char *filename;

	if (ltag == NULL) return DC_NOTOK;
	filename = template_filename(db->config, ltag);

	rec = config_new();
	if (rec->read(rec, filename) != DC_OK)
	{
		config_delete(rec);
		return NULL;
	}

	t = NEW(struct template);

	t->tag = STRDUP(rec->get(rec, "template::tag", 0));
	if (t->tag == 0)
	{
		template_delete(t);
		t = 0;
	}
	else
	{
		t->type = STRDUP(rec->get(rec, "template::type", "string"));
		t->defaultval = STRDUP(rec->get(rec, "template::default", 0));
		t->choices = STRDUP(rec->get(rec, "template::choices", 0));
		t->description = STRDUP(rec->get(rec, "template::description", 0));
		t->extended_description = STRDUP(rec->get(rec, "template::extended_description", 0));
	}

	config_delete(rec);

	return t;
}

static struct template *textdb_template_get(struct database *db, 
	const char *ltag)
{
	struct template *result;

	result = textdb_lookup_cached_template(db, ltag);
	if (!result && (result = textdb_template_get_real(db, ltag)))
	{
		struct db_cache *dbdata = db->data;
		result->next = dbdata->templates;
		dbdata->templates = result;
	}
	return result;
}

static int textdb_template_remove(struct database *db, const char *tag)
{
	char *filename;

	if (tag == NULL) return DC_NOTOK;

	textdb_remove_cached_template(db, tag);

	filename = template_filename(db->config, tag);
	if (unlink(filename) == 0)
		return DC_OK;
	else
		return DC_NOTOK;
}

static struct template *textdb_template_iterate(struct database *db,
	void **iter)
{
	DIR *dir;
	struct dirent *ent;

	if (*iter == NULL)
	{
		dir = opendir(db->config->get(db->config, 
			"database::driver::textdb::templatepath", 
			TEXTDB_TEMPLATE_PATH));
		if (dir == NULL)
			return NULL;
		*iter = dir;
	}
	else
	{
		dir = (DIR *)*iter;
	}

	if ((ent = readdir(dir)) == NULL)
	{
		closedir(dir);
		return NULL;
	}

	return textdb_template_get(db, ent->d_name);
}

static int textdb_question_add(struct database *db, struct question *q)
{
	FILE *outf;
	char *filename;

	if (q->tag == NULL) return DC_NOTOK;
	filename = question_filename(db->config, q->tag);
	
	if ((outf = fopen(filename, "w")) == NULL)
		return DC_NOTOK;

	fprintf(outf, "question {\n");
	fprintf(outf, "\tname \"%s\";\n", q->tag);

	fprintf(outf, "}\n");
	fclose(outf);
	
	return DC_OK;
}

static struct question *textdb_question_get(struct database *db, 
	const char *ltag)
{
	struct configuration *rec;
	struct question *q;
	char *filename;

	if (ltag == NULL) return DC_NOTOK;
	filename = question_filename(db->config, ltag);

	rec = config_new();
	if (rec->read(rec, filename) != DC_OK)
	{
		config_delete(rec);
		return NULL;
	}

	q = NEW(struct question);

	q->tag = STRDUP(rec->get(rec, "question::tag", 0));
	q->value = STRDUP(rec->get(rec, "question::value", 0));
	q->template = textdb_template_get(db,
		rec->get(rec, "question::template", 0));
	if (q->tag == 0 || q->value == 0 || q->template == 0)
	{
		question_delete(q);
		q = 0;
	}

	config_delete(rec);

	return q;
}

static int textdb_question_disown(struct database *db, const char *tag, 
	const char *owner)
{
	struct question *q = textdb_question_get(db, tag);
	if (q == NULL) return DC_NOTOK;
	question_owner_delete(q, owner);
	textdb_question_add(db, q);
	question_delete(q);
	return DC_OK;
}

static struct question *textdb_question_iterate(struct database *db,
	void **iter)
{
	DIR *dir;
	struct dirent *ent;

	if (*iter == NULL)
	{
		dir = opendir(db->config->get(db->config, 
			"database::driver::textdb::questionpath", 
			TEXTDB_QUESTION_PATH));
		if (dir == NULL)
			return NULL;
		*iter = dir;
	}
	else
	{
		dir = (DIR *)*iter;
	}

	if ((ent = readdir(dir)) == NULL)
	{
		closedir(dir);
		return NULL;
	}

	return textdb_question_get(db, ent->d_name);
}

struct database_module debconf_database_module =
{
	initialize: textdb_initialize,
	load: textdb_load,
	save: textdb_save,

	template_add: textdb_template_add,
	template_get: textdb_template_get,
	template_remove: textdb_template_remove,
	template_iterate: textdb_template_iterate,

	question_add: textdb_question_add,
	question_get: textdb_question_get,
	question_set: textdb_question_add,	/* no separate set method */
	question_disown: textdb_question_disown,
	question_iterate: textdb_question_iterate
};
