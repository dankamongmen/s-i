#include "common.h"
#include "configuration.h"
#include "database.h"
#include "frontend.h"
#include "question.h"

#include <dlfcn.h>
#include <string.h>

#define SETMETHOD(method) obj->method = (mod->method ? mod->method : frontend_ ## method)

static int frontend_add(struct frontend *obj, struct question *q)
{
	struct question **qlast;

	if (q == NULL) {
		fprintf(stderr, "WTF?\n");
	}

	for (qlast = &obj->questions; *qlast; qlast = &(*qlast)->next);
	*qlast = q;
	q->next = NULL;

	return 1;
}

static int frontend_go(struct frontend *obj)
{
	return 0;
}

static void frontend_clear(struct frontend *obj)
{
	struct question *q;
	
	while (obj->questions != NULL)
	{
		q = obj->questions;
		obj->questions = obj->questions->next;
		question_delete(q);
	}
}

static int frontend_initialize(struct frontend *obj, struct configuration *cfg)
{
	return DC_OK;
}

static int frontend_shutdown(struct frontend *obj)
{
	return DC_OK;
}

static unsigned long frontend_query_capability(struct frontend *f)
{
	return 0;
}

static void frontend_set_title(struct frontend *f, const char *title)
{
	DELETE(f->title);
	f->title = STRDUP(title);
}

struct frontend *frontend_new(struct configuration *cfg, struct database *db)
{
	struct frontend *obj = NULL;
	void *dlh;
	struct frontend_module *mod;
	char modlabel[256];
	const char *modname;

	if ((modname = cfg->get(cfg, "frontend::default::driver", 0)) == NULL)
		DIE("No frontend driver defined");

	snprintf(modlabel, sizeof(modlabel), "frontend::driver::%s::module",
		modname);
	
	modname = cfg->get(cfg, modlabel, 0);
	if ((dlh = dlopen(modname, RTLD_NOW)) == NULL)
		DIE("Cannot load frontend module %s", modname);

	if ((mod = (struct frontend_module *)dlsym(dlh, "debconf_frontend_module")) == NULL)
		DIE("Malformed frontend module %s", modname);
	
	obj = NEW(struct frontend);
	obj->handle = dlh;
	obj->data = NULL;
	obj->config = cfg;
	obj->db = db;

	SETMETHOD(initialize);
	SETMETHOD(shutdown);
	SETMETHOD(query_capability);
	SETMETHOD(set_title);
	SETMETHOD(add);
	SETMETHOD(go);
	SETMETHOD(clear);

	if (obj->initialize(obj, cfg) == 0)
	{
		frontend_delete(obj);
		return NULL;
	}

	obj->capability = obj->query_capability(obj);

	return obj;
}

void frontend_delete(struct frontend *obj)
{
	obj->shutdown(obj);
	dlclose(obj->handle);
	DELETE(obj->questions);
	DELETE(obj->capb);
	DELETE(obj->title);
	DELETE(obj);
}

