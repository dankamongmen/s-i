/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: commands.c
 *
 * Description: implementation of each command specified in the spec
 *
 * $Id: commands.c,v 1.11 2000/12/09 08:18:07 tausq Exp $
 *
 * cdebconf is (c) 2000 Randolph Chung and others under the following
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
#include "commands.h"
#include "frontend.h"
#include "database.h"
#include "question.h"
#include "template.h"
#include "strutl.h"

#define CHECKARGC(pred) \
	if (_command_checkargc(argc ## pred, out, outsize) == 0) return DC_OK

static int _command_checkargc(int pred, char *out, size_t outsize)
{
	if (!pred)
	{
		snprintf(out, outsize, "%u Incorrect number of arguments", 
			CMDSTATUS_SYNTAXERROR);
		return DC_NOTOK;
	}
	else
	{
		return DC_OK;
	}
}

int command_input(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	int visible = 1;
	struct question *q;
	char *qtag;
	char *priority;

	CHECKARGC(== 2);

	/* check question */
	/* check priority */

	priority = argv[1];
	qtag = argv[2];

	visible = (mod->frontend->interactive && mod->db->question_visible(mod->db, qtag, priority));

	
	if (visible)
	{
		q = mod->db->question_get(mod->db, qtag);
		if (!q) {
			snprintf(out, outsize, "%u No such question",
				CMDSTATUS_BADQUESTION);
			return DC_OK;
		}
		visible = mod->frontend->add(mod->frontend, q);
	}

	if (visible)
		snprintf(out, outsize, "%u Question will be asked",
			CMDSTATUS_SUCCESS);
	else
		snprintf(out, outsize, "%u Question skipped",
			CMDSTATUS_INPUTINVISIBLE);
	return DC_OK;
}

int command_clear(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	CHECKARGC(== 0);

	mod->frontend->clear(mod->frontend);
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
}

int command_version(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	int ver;

	CHECKARGC(== 1);

	ver = atoi(argv[1]);

	if (ver < DEBCONF_VERSION)
		snprintf(out, outsize, "%u Version too low (%d)",
			CMDSTATUS_BADVERSION, ver);
	else if (ver > DEBCONF_VERSION)
		snprintf(out, outsize, "%u Version too high (%d)",
			CMDSTATUS_BADVERSION, ver);
	else
		snprintf(out, outsize, "%u %.0f", CMDSTATUS_SUCCESS,
			DEBCONF_VERSION);
	return DC_OK;
}

int command_capb(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
	// return DC_NOTOK;
}

int command_title(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	char buf[1024] = {0};
	int i;
	for (i = 1; i <= argc; i++)
		strvacat(buf, sizeof(buf), argv[i], " ", 0);	

	mod->frontend->set_title(mod->frontend, buf);
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
}

int command_beginblock(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
}

int command_endblock(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
	return DC_OK;
}

int command_go(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	CHECKARGC(== 0);

	if (mod->frontend->go(mod->frontend) == CMDSTATUS_GOBACK)
		snprintf(out, outsize, "%u backup", CMDSTATUS_GOBACK);
	else
		snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);

	mod->frontend->clear(mod->frontend);

	return DC_OK;
}

int command_get(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	CHECKARGC(== 1);

	q = mod->db->question_get(mod->db, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
		snprintf(out, outsize, "%u %s",
			CMDSTATUS_SUCCESS, q->value ? q->value : "");

	return DC_OK;
}

int command_set(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	int i;
	char buf[1024];

	CHECKARGC(>= 2);

	q = mod->db->question_get(mod->db, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		buf[0] = 0;
		for (i = 2; i <= argc; i++)
			strvacat(buf, sizeof(buf), argv[i], 0);	

		question_setvalue(q, buf);

		if (mod->db->question_set(mod->db, q) != 0)
			snprintf(out, outsize, "%u value set",
				CMDSTATUS_SUCCESS);
		else
			snprintf(out, outsize, "%u cannot set value",
				CMDSTATUS_INTERNALERROR);
	}

	return DC_OK;
}

int command_reset(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;

	CHECKARGC(== 1);

	q = mod->db->question_get(mod->db, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		DELETE(q->value);
		q->value = STRDUP(q->defaultval);
		q->flags |= DC_QFLAG_SEEN;

		if (mod->db->question_set(mod->db, q) != 0)
			snprintf(out, outsize, "%u value reset",
				CMDSTATUS_SUCCESS);
		else
			snprintf(out, outsize, "%u cannot reset value",
				CMDSTATUS_INTERNALERROR);
	}

	return DC_OK;
}

int command_subst(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	struct questionvariable *var, *prevvar;
	char *variable;
	int i;
	char buf[1024];

	CHECKARGC(>= 2);

	variable = argv[2];
	q = mod->db->question_get(mod->db, argv[1]);

	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		buf[0] = 0;
		for (i = 3; i <= argc; i++)
			strvacat(buf, sizeof(buf), argv[i], " ", 0);	

		question_variable_add(q, variable, buf);

		if (mod->db->question_set(mod->db, q) != 0)
			snprintf(out, outsize, "%u variable substituted",
				CMDSTATUS_SUCCESS);
		else
			snprintf(out, outsize, "%u cannot set variable",
				CMDSTATUS_INTERNALERROR);
	}

	return DC_OK;
}
int command_register(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	struct template *t;
	CHECKARGC(== 2);
	
	return DC_NOTOK;
}

int command_unregister(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	CHECKARGC(== 1);

	mod->db->question_disown(mod->db, argv[1], mod->owner);
	snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);

	return DC_OK;
}

int command_purge(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	mod->db->question_disownall(mod->db, mod->owner);
	return DC_OK;
}

int command_metaget(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	char *field;

	CHECKARGC(== 2);
	q = mod->db->question_get(mod->db, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		field = argv[2];
		
		/* 
		 * the spec is very vague here, what fields are we supposed 
		 * to recognize? default? localized description fields?
		 * name of the question? type of the question?
		 */
		if (strcmp(field, "value") == 0)
			snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS, q->value);
		else if (strcmp(field, "description") == 0)
			snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS, question_description(q));
		else if (strcmp(field, "extended_description") == 0)
			snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS, question_extended_description(q));
		else if (strcmp(field, "choices") == 0)
			snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS, question_choices(q));
		else
			snprintf(out, outsize, "%u %s does not exist", CMDSTATUS_BADPARAM, field);
	}

	return DC_OK;
}

int command_fget(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	char *field;

	CHECKARGC(== 2);
	q = mod->db->question_get(mod->db, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		field = argv[2];
		
		/* 
		 * the spec is very vague here, what fields are we supposed 
		 * to recognize?
		 */

		/* isdefault is for backward compability only */
		if (strcmp(field, "seen") == 0 ||
		    strcmp(field, "isdefault") == 0)
			snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS,
				((q->flags & DC_QFLAG_SEEN) ? "true" : "false"));
		else
			snprintf(out, outsize, "%u %s does not exist", CMDSTATUS_BADPARAM, field);
	}

	return DC_OK;
}

int command_fset(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	char *field;

	CHECKARGC(== 3);
	q = mod->db->question_get(mod->db, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		field = argv[2];
		if (strcmp(field, "seen") == 0 ||
		    strcmp(field, "isdefault") == 0)
		{
			q->flags &= ~DC_QFLAG_SEEN;
			if (strcmp(argv[3], "true") == 0)
				q->flags |= DC_QFLAG_SEEN;
			mod->db->question_set(mod->db, q);
			snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
		}
		else
			snprintf(out, outsize, "%u %s does not exist", CMDSTATUS_BADPARAM, field);
	}
	return DC_OK;
}

int command_exist(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	struct question *q;
	CHECKARGC(== 1);

	q = mod->db->question_get(mod->db, argv[1]);
	if (q)
	{
		question_deref(q);
		snprintf(out, outsize, "%u true", CMDSTATUS_SUCCESS);
	}
	else
	{
		snprintf(out, outsize, "%u false", CMDSTATUS_SUCCESS);
	}
	return DC_OK;
}

int command_stop(struct confmodule *mod, int argc, char **argv,
	char *out, size_t outsize)
{
	CHECKARGC(== 0);
	return DC_OK;
}
