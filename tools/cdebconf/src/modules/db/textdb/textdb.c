#include "common.h"
#include "configuration.h"
#include "database.h"
#include "textdb.h"
#include "template.h"
#include "question.h"
#include "strutl.h"

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

static char *unescapestr(const char *in)
{
	static char buf[8192];
	char *p = (char *)in;
	if (in == 0) return 0;
	strunescape(p, buf, sizeof(buf));
	return buf;
}

static char *escapestr(const char *in)
{
	static char buf[8192];
	if (in == 0) return 0;
	strescape(in, buf, sizeof(buf));
	return buf;
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

/*
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
*/

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

static int textdb_template_set(struct database *db, struct template *t)
{
	FILE *outf;
	char *filename;
	struct language_description *langdesc;

	if (t->tag == NULL) return DC_NOTOK;
	filename = template_filename(db->config, t->tag);
	
	if ((outf = fopen(filename, "w")) == NULL)
		return DC_NOTOK;

	fprintf(outf, "template {\n");

	fprintf(outf, "\ttag \"%s\";\n", escapestr(t->tag));
	fprintf(outf, "\ttype \"%s\";\n", escapestr(t->type));
	if (t->defaultval != NULL)
		fprintf(outf, "\tdefault \"%s\";\n", escapestr(t->defaultval));
	if (t->choices != NULL)
		fprintf(outf, "\tchoices \"%s\";\n", escapestr(t->choices));
	if (t->description != NULL)
		fprintf(outf, "\tdescription \"%s\";\n", escapestr(t->description));
	if (t->extended_description != NULL)
		fprintf(outf, "\textended_description \"%s\";\n", escapestr(t->extended_description));

	langdesc = t->localized_descriptions;
	while (langdesc) 
	{
		if (langdesc->description != NULL) 
			fprintf(outf, "\tdescription-%s \"%s\";\n", langdesc->language, escapestr(langdesc->description));
		if (langdesc->extended_description != NULL)
			fprintf(outf, "\textended_description-%s \"%s\";\n", langdesc->language, escapestr(langdesc->extended_description));
		if (langdesc->choices != NULL) 
			fprintf(outf, "\tchoices-%s \"%s\";\n", langdesc->language, escapestr(langdesc->choices));

		langdesc = langdesc->next;
	}

	fprintf(outf, "};\n");
	fclose(outf);
	
	return DC_OK;
}

static struct template *textdb_template_get_real(struct database *db, 
	const char *ltag)
{
	struct configuration *rec;
	struct template *t;
	char *filename;
	char a,b, langtmp[128];
	const char *tmp;

	if (ltag == NULL) return DC_NOTOK;
	filename = template_filename(db->config, ltag);

	rec = config_new();
	if (rec->read(rec, filename) != DC_OK)
	{
		config_delete(rec);
		return NULL;
	}

	t = template_new(0);

	t->tag = STRDUP(unescapestr(rec->get(rec, "template::tag", 0)));
	if (t->tag == 0)
	{
		template_deref(t);
		t = 0;
	}
	else
	{
		t->type = STRDUP(unescapestr(rec->get(rec, "template::type", "string")));
		t->defaultval = STRDUP(unescapestr(rec->get(rec, "template::default", 0)));
		t->choices = STRDUP(unescapestr(rec->get(rec, "template::choices", 0)));
		t->description = STRDUP(unescapestr(rec->get(rec, "template::description", 0)));
		t->extended_description = STRDUP(unescapestr(rec->get(rec, "template::extended_description", 0)));

		/* We can't ask for all the localized descriptions so we need to
		 * brute-force this.
		 * This is stupid and ugly and should be fixed, FIXME
		 */
		for (a = 'a'; a <= 'z'; a++)
			for (b = 'a'; b <= 'z'; b++)
			{
				sprintf(langtmp, "template::description-%c%c", a, b);
				tmp = rec->get(rec, langtmp, 0);
				if (tmp)
				{
					struct language_description *langdesc = malloc(sizeof(struct language_description));
					memset(langdesc,0,sizeof(struct language_description));
					langdesc->language = malloc(3);
					sprintf(langdesc->language,"%c%c", a, b);
					langdesc->description = STRDUP(unescapestr(tmp));
					sprintf(langtmp,"template::extended_description-%c%c", a, b);
					langdesc->extended_description = STRDUP(unescapestr(rec->get(rec, langtmp, 0)));
					sprintf(langtmp,"template::choices-%c%c", a, b);
					langdesc->choices = STRDUP(unescapestr(rec->get(rec, langtmp, 0)));
					if (strcmp(langdesc->choices,"") == 0)
					{
						free(langdesc->choices);
						langdesc->choices = NULL;
					}
					langdesc->next = t->localized_descriptions;
					t->localized_descriptions = langdesc;
				}
			}
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

static int textdb_question_set(struct database *db, struct question *q)
{
	FILE *outf;
	char *filename;
	struct questionvariable *var;
	struct questionowner *owner;

	if (q->tag == NULL) return DC_NOTOK;
	filename = question_filename(db->config, q->tag);
	
	if ((outf = fopen(filename, "w")) == NULL)
		return DC_NOTOK;

	fprintf(outf, "question {\n");
	fprintf(outf, "\ttag \"%s\";\n", escapestr(q->tag));
	fprintf(outf, "\tvalue \"%s\";\n", (q->value ? escapestr(q->value) : ""));
	if (q->defaultval)
		fprintf(outf, "\tdefault \"%s\";\n", escapestr(q->defaultval));
	fprintf(outf, "\tflags 0x%08X;\n", q->flags);
	fprintf(outf, "\ttemplate \"%s\";\n", escapestr(q->template->tag));
	if ((var = q->variables))
	{
		fprintf(outf, "\tvariables {\n");
		do {
			/* escapestr uses a static buf, so we do this in
			 * two steps */
			fprintf(outf, "\t\t%s ", escapestr(var->variable));
			fprintf(outf, "\"%s\";\n", escapestr(var->value));
		} while ((var = var->next));
		fprintf(outf, "\t};\n");
	}
	if ((owner = q->owners))
	{
		fprintf(outf, "\towners:: {\n");
		do {
			fprintf(outf, "\t\t\"%s\";\n", escapestr(owner->owner));
		} while ((owner = owner->next));
		fprintf(outf, "\t};\n");
	}

	fprintf(outf, "};\n");
	fclose(outf);
	
	return DC_OK;
}

static struct question *textdb_question_get(struct database *db, 
	const char *ltag)
{
	struct configuration *rec;
	struct question *q;
	char *filename;
	struct configitem *node;

	if (ltag == NULL) return DC_NOTOK;
	filename = question_filename(db->config, ltag);

	rec = config_new();
	if (rec->read(rec, filename) != DC_OK)
	{
		config_delete(rec);
		return NULL;
	}

	q = question_new(0);

	q->tag = STRDUP(unescapestr(rec->get(rec, "question::tag", 0)));
	q->value = STRDUP(unescapestr(rec->get(rec, "question::value", 0)));
	q->defaultval = STRDUP(unescapestr(rec->get(rec, "question::default", 0)));
	q->flags = rec->geti(rec, "question::flags", 0);
	q->template = textdb_template_get(db,
		unescapestr(rec->get(rec, "question::template", 0)));

	/* TODO: variables and owners */
	if ((node = rec->tree(rec, "question::variables")) != 0)
	{
		node = node->child;
		for (; node != 0; node = node->next)
			question_variable_add(q, node->tag, node->value);
	}

	if ((node = rec->tree(rec, "question::owners")) != 0)
	{
		node = node->child;
		for (; node != 0; node = node->next)
			if (node->tag && node->tag[0] != 0 && node->tag[0] != ':')
				question_owner_add(q, node->tag);
	}

	if (q->tag == 0 || q->value == 0 || q->template == 0)
	{
		question_deref(q);
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
	textdb_question_set(db, q);
	question_deref(q);
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

	template_set: textdb_template_set,
	template_get: textdb_template_get,
	template_remove: textdb_template_remove,
	template_iterate: textdb_template_iterate,

	question_get: textdb_question_get,
	question_set: textdb_question_set,
	question_disown: textdb_question_disown,
	question_iterate: textdb_question_iterate
};
