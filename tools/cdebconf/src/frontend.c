/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: frontend.c
 *
 * Description: debconf frontend interface routines
 *
 * $Id: frontend.c,v 1.10 2002/05/30 11:04:50 tfheen Rel $
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
#include "configuration.h"
#include "database.h"
#include "frontend.h"
#include "question.h"

#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

#define SETMETHOD(method) obj->method = (mod->method ? mod->method : frontend_ ## method)

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */
static int frontend_add(struct frontend *obj, struct question *q)
{
	struct question *qlast;
	ASSERT(q != NULL);
	ASSERT(q->prev == NULL);
	ASSERT(q->next == NULL);

	qlast = obj->questions;
	if (qlast == NULL)
	{
		obj->questions = q;
	}
	else
	{
		while (qlast->next != NULL)
			qlast = qlast->next;
		qlast->next = q;
		q->prev = qlast;
	}

	question_ref(q);

	return DC_OK;
}

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */
static int frontend_go(struct frontend *obj)
{
	return 0;
}

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */
static void frontend_clear(struct frontend *obj)
{
	struct question *q;
	
	while (obj->questions != NULL)
	{
		q = obj->questions;
		obj->questions = obj->questions->next;
		question_deref(q);
	}
}

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */
static int frontend_initialize(struct frontend *obj, struct configuration *cfg)
{
	return DC_OK;
}

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */
static int frontend_shutdown(struct frontend *obj)
{
	return DC_OK;
}

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */
static unsigned long frontend_query_capability(struct frontend *f)
{
	return 0;
}

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */
static void frontend_set_title(struct frontend *f, const char *title)
{
	DELETE(f->title);
	f->title = STRDUP(title);
}

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */
static int frontend_cangoback(struct frontend *ui, struct question *q)
{
	return 1;
}

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */
static int frontend_cangoforward(struct frontend *ui, struct question *q)
{
	return 1;
}

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */

struct frontend *frontend_new(struct configuration *cfg, struct database *db)
{
	struct frontend *obj = NULL;
	void *dlh;
	struct frontend_module *mod;
	char modlabel[256];
	const char *modname;

	modname = getenv("DEBCONF_FRONTEND");
	if (modname == NULL)
		if ((modname = cfg->get(cfg, "frontend::default::driver", 0)) == NULL)
			DIE("No frontend driver defined");

	snprintf(modlabel, sizeof(modlabel), "frontend::driver::%s::module",
		modname);

	modname = cfg->get(cfg, modlabel, 0);
	if ((dlh = dlopen(modname, RTLD_NOW)) == NULL)
		DIE("Cannot load frontend module %s: %s", modname, dlerror());

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
	SETMETHOD(cangoback);
	SETMETHOD(cangoforward);

	if (obj->initialize(obj, cfg) == 0)
	{
		frontend_delete(obj);
		return NULL;
	}

	obj->capability = obj->query_capability(obj);
	INFO(INFO_VERBOSE, "Capability: 0x%08X\n", obj->capability);

	return obj;
}

/*
 * Function:
 * Input:
 * Output:
 * Description:
 * Assumptions:
 */
void frontend_delete(struct frontend *obj)
{
	obj->shutdown(obj);
	dlclose(obj->handle);
	DELETE(obj->questions);
	DELETE(obj->capb);
	DELETE(obj->title);
	DELETE(obj);
}

