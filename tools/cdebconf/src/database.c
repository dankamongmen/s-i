/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: database.c
 *
 * Description: database interface routines
 *
 * $Id: database.c,v 1.10 2002/05/30 11:11:29 tfheen Rel $
 *
 * cdebconf is (c) 2000-2001 Randolph Chung and others under the following
 * license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ***********************************************************************/
#include "common.h"
#include "database.h"
#include "configuration.h"
#include "question.h"
#include "template.h"
#include "priority.h"

#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

#define SETMETHOD(method) db->method = (mod->method ? mod->method : database_ ## method)

static int database_initialize(struct database *db, struct configuration *cfg)
{
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

static int database_template_set(struct database *db, struct template *t)
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
	struct question *q, *q2 = 0;
	int ret = DC_YES;

	if (getenv("DEBCONF_PRIORITY") != NULL &&
	    priority_compare(priority, getenv("DEBCONF_PRIORITY")) < 0)
		return DC_NO;

	if ((q = db->question_get(db, "debconf/priority")) != NULL)
	{
		if (priority_compare(priority, q->value) < 0)
			ret = DC_NO;
		question_deref(q);
		if (ret != DC_YES) return ret;
	}
	
	if ((q = db->question_get(db, name)) != NULL &&
	    (q->flags & DC_QFLAG_SEEN) != 0)
	{
		if ((q2 = db->question_get(db, "debconf/showold")) != NULL &&
			strcmp(q2->value, "false") == 0)
			ret = DC_NO;
	}
	question_deref(q);
	question_deref(q2);
	return ret;
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

	modname = getenv("DEBCONF_DB");
	if (modname == NULL)
		if ((modname = cfg->get(cfg, "database::default::driver", 0)) == NULL)
			DIE("No database driver defined");

	snprintf(modlabel, sizeof(modlabel), "database::driver::%s::module",
		modname);
	
	modname = cfg->get(cfg, modlabel, 0);
	if ((dlh = dlopen(modname, RTLD_NOW)) == NULL)
		DIE("Cannot load database module %s: %s", modname, dlerror());

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
	SETMETHOD(template_set);
	SETMETHOD(template_get);
	SETMETHOD(template_remove);
	SETMETHOD(template_lock);
	SETMETHOD(template_unlock);
	SETMETHOD(template_iterate);
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

