#include "common.h"
#include "database.h"
#include "configuration.h"
#include "question.h"
#include "template.h"
#include "priority.h"

#include <dlfcn.h>

#define SETMETHOD(method) db->method = (mod->method ? mod->method : database_ ## method)

static int database_initialize(struct database *db, struct configuration *cfg)
{
	printf("in initialize\n");
	return DC_OK;
}

static int database_shutdown(struct database *db)
{
	return DC_OK;
}

static int database_load(struct database *db)
{
	return DC_OK;
}

static int database_save(struct database *db)
{
	return DC_OK;
}

static int database_template_add(struct database *db, struct template *t)
{
	return DC_NOTIMPL;
}

static struct template *database_template_get(struct database *db, 
	const char *name)
{
	return 0;
}

static int database_template_remove(struct database *db, const char *name)
{
	return DC_NOTIMPL;
}

static int database_template_lock(struct database *db, const char *name)
{
	return DC_NOTIMPL;
}

static int database_template_unlock(struct database *db, const char *name)
{
	return DC_NOTIMPL;
}

static struct template *database_template_iterate(struct database *db, 
	void **iter)
{
	return 0;
}

static int database_question_add(struct database *db, struct question *q)
{
	return DC_NOTIMPL;
}

static struct question *database_question_get(struct database *db, 
	const char *name)
{
	return 0;
}

static int database_question_set(struct database *db, struct question *q)
{
	return DC_NOTIMPL;
}

static int database_question_disown(struct database *db, const char *name, 
	const char *owner)
{
	return DC_NOTIMPL;
}

static int database_question_disownall(struct database *db, const char *owner)
{
	struct question *q;
	void *iter = 0;

	while ((q = db->question_iterate(db, &iter)))
	{
		db->question_disown(db, q->tag, owner);
	}
	return 0;
}

static int database_question_lock(struct database *db, const char *name)
{
	return DC_NOTIMPL;
}

static int database_question_unlock(struct database *db, const char *name)
{
	return DC_NOTIMPL;
}

static int database_question_visible(struct database *db, const char *name,
	const char *priority)
{
	struct question *q = db->question_get(db, "debconf/priority");
	if (q != NULL)
	{
		if (priority_compare(priority, q->value) < 0)
			return DC_NO;
		question_delete(q);
	}
	return DC_YES;
}

static struct question *database_question_iterate(struct database *db,
	void **iter)
{
	return 0;
}

struct database *database_new(struct configuration *cfg)
{
	struct database *db;
	void *dlh;
	struct database_module *mod;
	char modlabel[256];
	const char *modname;

	if ((modname = cfg->get(cfg, "database::default::driver", 0)) == NULL)
		DIE("No database driver defined");

	snprintf(modlabel, sizeof(modlabel), "database::driver::%s::module",
		modname);
	
	modname = cfg->get(cfg, modlabel, 0);
	if ((dlh = dlopen(modname, RTLD_NOW)) == NULL)
		DIE("Cannot load database module %s", modname);

	if ((mod = (struct database_module *)dlsym(dlh, "debconf_database_module")) == NULL)
		DIE("Malformed database module %s", modname);

	db = NEW(struct database);
	db->handle = dlh;
	db->modname = modname;
	db->data = NULL;
	db->config = cfg;

	SETMETHOD(initialize);
	SETMETHOD(shutdown);
	SETMETHOD(load);
	SETMETHOD(save);
	SETMETHOD(template_add);
	SETMETHOD(template_get);
	SETMETHOD(template_remove);
	SETMETHOD(template_lock);
	SETMETHOD(template_unlock);
	SETMETHOD(template_iterate);
	SETMETHOD(question_add);
	SETMETHOD(question_get);
	SETMETHOD(question_set);
	SETMETHOD(question_disown);
	SETMETHOD(question_disownall);
	SETMETHOD(question_lock);
	SETMETHOD(question_unlock);
	SETMETHOD(question_visible);
	SETMETHOD(question_iterate);

	if (db->initialize(db, cfg) == 0)
	{
		database_delete(db);
		return NULL;
	}

	return db;
}

void database_delete(struct database *db)
{
	db->shutdown(db);
	dlclose(db->handle);

	DELETE(db);
}

