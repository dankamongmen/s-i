/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: database.c
 *
 * Description: database interface routines
 *
 * $Id: database.c,v 1.12 2002/07/31 21:48:55 tfheen Exp $
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

/****************************************************************************
 *
 * Template database 
 *
 ***************************************************************************/

static int template_db_initialize(struct template_db *db, struct configuration *cfg)
{
    return DC_OK;
}

static int template_db_shutdown(struct template_db *db)
{
    return DC_OK;
}

static int template_db_load(struct template_db *db)
{
    return DC_OK;
}

static int template_db_save(struct template_db *db)
{
    return DC_OK;
}

static int template_db_set(struct template_db *db, struct template *t)
{
	return DC_NOTIMPL;
}

static struct template *template_db_get(struct template_db *db, 
	const char *name)
{
	return 0;
}

static int template_db_remove(struct template_db *db, const char *name)
{
	return DC_NOTIMPL;
}

static int template_db_lock(struct template_db *db, const char *name)
{
	return DC_NOTIMPL;
}

static int template_db_unlock(struct template_db *db, const char *name)
{
	return DC_NOTIMPL;
}

static struct template *template_db_iterate(struct template_db *db, 
	void **iter)
{
	return 0;
}

struct template_db *template_db_new(struct configuration *cfg)
{
	struct template_db *db;
	void *dlh;
	struct template_db_module *mod;
	char tmp[256];
	const char *modpath, *modname, *driver;

    modname = cfg->get(cfg, "global::default::template", getenv("DEBCONF_TEMPLATE"));

    if (modname == NULL)
		DIE("No template database instance defined");

    modpath = cfg->get(cfg, "global::module_path::database", 0);
    if (modpath == NULL)
        DIE("Database module path not defined (global::module_path::database)");

    snprintf(tmp, sizeof(tmp), "template::instance::%s::driver",
        modname);
    driver = cfg->get(cfg, tmp, 0);

    if (driver == NULL)
        DIE("Template instance driver not defined (%s)", tmp);

    snprintf(tmp, sizeof(tmp), "%s/%s.so", modpath, driver);
	if ((dlh = dlopen(tmp, RTLD_NOW)) == NULL)
		DIE("Cannot load template database module %s: %s", tmp, dlerror());

	if ((mod = (struct template_db_module *)dlsym(dlh, "debconf_template_db_module")) == NULL)
		DIE("Malformed template database module %s", modname);

	db = NEW(struct template_db);
	db->handle = dlh;
	db->modname = modname;
	db->data = NULL;
	db->config = cfg;
    snprintf(db->configpath, sizeof(db->configpath), 
        "template::instance::%s", modname);

    memcpy(&db->methods, mod, sizeof(struct template_db_module));

#define SETMETHOD(method) if (db->methods.method == NULL) db->methods.method = template_db_##method

	SETMETHOD(initialize);
	SETMETHOD(shutdown);
	SETMETHOD(load);
	SETMETHOD(save);
	SETMETHOD(set);
	SETMETHOD(get);
	SETMETHOD(remove);
	SETMETHOD(lock);
	SETMETHOD(unlock);
	SETMETHOD(iterate);

#undef SETMETHOD

	if (db->methods.initialize(db, cfg) == 0)
	{
		template_db_delete(db);
		return NULL;
	}

	return db;
}

void template_db_delete(struct template_db *db)
{
	db->methods.shutdown(db);
	dlclose(db->handle);

	DELETE(db);
}

/****************************************************************************
 *
 * Config database 
 *
 ***************************************************************************/

static int question_db_initialize(struct question_db *db, struct configuration *cfg)
{
    return DC_OK;
}

static int question_db_shutdown(struct question_db *db)
{
    return DC_OK;
}

static int question_db_load(struct question_db *db)
{
    return DC_OK;
}

static int question_db_save(struct question_db *db)
{
    return DC_OK;
}

static int question_db_set(struct question_db *db, struct question *t)
{
	return DC_NOTIMPL;
}

static struct question *question_db_get(struct question_db *db, 
	const char *name)
{
	return 0;
}

static int question_db_disown(struct question_db *db, const char *name, 
	const char *owner)
{
	return DC_NOTIMPL;
}

static int question_db_disownall(struct question_db *db, const char *owner)
{
	struct question *q;
	void *iter = 0;

	while ((q = db->methods.iterate(db, &iter)))
	{
		db->methods.disown(db, q->tag, owner);
	}
	return 0;
}

static int question_db_lock(struct question_db *db, const char *name)
{
	return DC_NOTIMPL;
}

static int question_db_unlock(struct question_db *db, const char *name)
{
	return DC_NOTIMPL;
}

static int question_db_is_visible(struct question_db *db, const char *name,
	const char *priority)
{
	struct question *q = 0, *q2 = 0;
	struct configuration *config = db->config;
    const char *wantprio = NULL;
	int ret = DC_YES;

    /* priority can either come from the command line, environment
     * or from debconf configuration
     */
    wantprio = config->get(config, "_cmdline::priority", NULL);

    if (wantprio == NULL)
	    wantprio = getenv("DEBCONF_PRIORITY");

    if (wantprio == NULL)
	    if ((q = db->methods.get(db, "debconf/priority")) != NULL)
            wantprio = q->value;

    /* error; no priority specified -- last resort fallback to medium */
    if (wantprio == NULL || strlen(wantprio) == 0)
        wantprio = "medium";
    else if (priority_compare(priority, wantprio) < 0)
		ret = DC_NO;

    if (q != NULL)
		question_deref(q);

	if (ret != DC_YES)
        return ret;
	
	if ((q = db->methods.get(db, name)) != NULL &&
	    (q->flags & DC_QFLAG_SEEN) != 0)
	{
		if ((q2 = db->methods.get(db, "debconf/showold")) != NULL &&
			strcmp(q2->value, "false") == 0)
			ret = DC_NO;
	}
	question_deref(q);
	question_deref(q2);
	return ret;
}

static struct question *question_db_iterate(struct question_db *db,
	void **iter)
{
	return 0;
}

struct question_db *question_db_new(struct configuration *cfg, struct template_db *tdb)
{
	struct question_db *db;
	void *dlh;
	struct question_db_module *mod;
	char tmp[256];
	const char *modpath, *modname, *driver;

    modname = getenv("DEBCONF_CONFIG");
    if (modname == NULL)
        modname = cfg->get(cfg, "global::default::config", 0);

    if (modname == NULL)
		DIE("No question database instance defined");

    modpath = cfg->get(cfg, "global::module_path::database", 0);
    if (modpath == NULL)
        DIE("Database module path not defined (global::module_path::database)");

    snprintf(tmp, sizeof(tmp), "config::instance::%s::driver",
        modname);
    driver = cfg->get(cfg, tmp, 0);

    if (driver == NULL)
        DIE("Config instance driver not defined (%s)", tmp);

    snprintf(tmp, sizeof(tmp), "%s/%s.so", modpath, driver);
	if ((dlh = dlopen(tmp, RTLD_NOW)) == NULL)
		DIE("Cannot load config database module %s: %s", tmp, dlerror());

	if ((mod = (struct question_db_module *)dlsym(dlh, "debconf_question_db_module")) == NULL)
		DIE("Malformed config database module %s", modname);

	db = NEW(struct question_db);
	db->handle = dlh;
	db->modname = modname;
	db->data = NULL;
	db->config = cfg;
    db->tdb = tdb;
    snprintf(db->configpath, sizeof(db->configpath), 
        "config::instance::%s", modname);

    memcpy(&db->methods, mod, sizeof(struct question_db_module));

#define SETMETHOD(method) if (db->methods.method == NULL) db->methods.method = question_db_##method

	SETMETHOD(initialize);
	SETMETHOD(shutdown);
	SETMETHOD(load);
	SETMETHOD(save);
	SETMETHOD(set);
	SETMETHOD(get);
	SETMETHOD(disown);
	SETMETHOD(disownall);
	SETMETHOD(lock);
	SETMETHOD(unlock);
	SETMETHOD(is_visible);
	SETMETHOD(iterate);

#undef SETMETHOD

	if (db->methods.initialize(db, cfg) == 0)
	{
		question_db_delete(db);
		return NULL;
	}

	return db;
}

void question_db_delete(struct question_db *db)
{
	db->methods.shutdown(db);
	dlclose(db->handle);

	DELETE(db);
}

