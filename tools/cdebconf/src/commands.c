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
	return DC_NOTOK;
}

int command_title(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	mod->frontend->title = strdup(argv[1]);
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
		q->flags |= DC_QFLAG_ISDEFAULT;

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
			strvacat(buf, sizeof(buf), argv[i], 0);	

		/* TODO */

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
	char field[1024];

	CHECKARGC(== 2);

	q = mod->db->question_get(mod->db, argv[1]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s does not exist",
			CMDSTATUS_BADQUESTION, argv[1]);
	else
	{
		field[0] = 0;

		snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS, field);
	}

	return DC_OK;
}

int command_fget(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	char buf[256];
	if (argc == 2)
	{
		snprintf(buf, sizeof(buf), "flag_%s", argv[2]);
		argv[2] = buf;
	}
	return command_metaget(mod, argc, argv, out, outsize);
}

int command_fset(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	return DC_NOTOK;
}

int command_exist(struct confmodule *mod, int argc, char **argv, 
	char *out, size_t outsize)
{
	return DC_NOTOK;
}

int command_stop(struct confmodule *mod, int argc, char **argv,
	char *out, size_t outsize)
{
	CHECKARGC(== 0);
	return DC_OK;
}
