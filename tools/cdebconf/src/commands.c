#include "common.h"
#include "commands.h"
#include "frontend.h"
#include "database.h"
#include "question.h"
#include "template.h"
#include "strutl.h"

#define CHECKARGC(pred) \
	if (_command_checkargc(argc pred, out, outsize) == DC_NOTOK) \
		return DC_OK


/**
 * @brief Checks to see if a given predicate is true; and if not
 *        write an appropriate message into the output buffer.
 *        Used in conjunction with the CHECKARGC macro above
 * @param int pred - predicate
 * @param char *out - output buffer
 * @param size_t outsize - output buffer size
 * @return int - DC_OK if pred is true, DC_NOTOK otherwise
 * @warning  static, to be used by command_* functions below
 */
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

int
command_input(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    int visible = 1;
    struct question *q = 0;
    char *qtag;
    char *priority;
    char *argv[3];
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(== 2);

    priority = argv[0];
    qtag = argv[1];

    q = mod->questions->methods.get(mod->questions, qtag);
    if (q == NULL) 
    {
        snprintf(out, outsize, "%u \"%s\" doesn't exist",
                CMDSTATUS_BADQUESTION, qtag);
        return DC_OK;
    }

    /* check priority */
    visible = (mod->frontend->interactive && 
            mod->questions->methods.is_visible(mod->questions, qtag, priority));

    if (visible)
        visible = mod->frontend->methods.add(mod->frontend, q);

    if (q->priority != NULL)
        free(q->priority);
    q->priority = strdup(priority);

    if (visible) 
        snprintf(out, outsize, "%u question will be asked",
                CMDSTATUS_SUCCESS);
    else
        snprintf(out, outsize, "%u question skipped",
                CMDSTATUS_INPUTINVISIBLE);
    question_deref(q);
    return DC_OK;
}

int
command_clear(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	char *argv[3];
	int argc;

	argc = strcmdsplit(arg, argv, DIM(argv));
	CHECKARGC(== 0);

	mod->frontend->methods.clear(mod->frontend);
	snprintf(out, outsize, "%u", CMDSTATUS_SUCCESS);
	return DC_OK;
}

int
command_version(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	int ver;
	char *argv[3];
	int argc;

	argc = strcmdsplit(arg, argv, DIM(argv));
	CHECKARGC(== 1);

	ver = atoi(argv[0]);

	if (ver < DEBCONF_VERSION)
		snprintf(out, outsize, "%u Version too low (%d)",
			CMDSTATUS_BADVERSION, ver);
	else if (ver > DEBCONF_VERSION)
		snprintf(out, outsize, "%u Version too high (%d)",
			CMDSTATUS_BADVERSION, ver);
	else
		snprintf(out, outsize, "%u %.1f", CMDSTATUS_SUCCESS,
			DEBCONF_VERSION);
	return DC_OK;
}

int
command_capb(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	int i;
	char *argv[32];
	int argc;

	argc = strcmdsplit(arg, argv, DIM(argv));
	/* FIXME: frontend.h should provide a method to prevent direct
	 *        access to capability membre */
	mod->frontend->capability = 0;
	for (i = 0; i < argc; i++)
		if (strcmp(argv[i], "backup") == 0)
			mod->frontend->capability |= DCF_CAPB_BACKUP;

	snprintf(out, outsize, "%u multiselect backup", CMDSTATUS_SUCCESS);
	return DC_OK;
}

int
command_title(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	mod->frontend->methods.set_title(mod->frontend, arg);
	snprintf(out, outsize, "%u", CMDSTATUS_SUCCESS);
	return DC_OK;
}

int
command_beginblock(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	snprintf(out, outsize, "%u", CMDSTATUS_SUCCESS);
	return DC_OK;
}

int
command_endblock(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	snprintf(out, outsize, "%u", CMDSTATUS_SUCCESS);
	return DC_OK;
}

int
command_go(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	char *argv[3];
	int argc;

	argc = strcmdsplit(arg, argv, DIM(argv) - 1);
	CHECKARGC(== 0);
	if (mod->frontend->methods.go(mod->frontend) == CMDSTATUS_GOBACK)
	{
		snprintf(out, outsize, "%u backup", CMDSTATUS_GOBACK);
		mod->update_seen_questions(mod, STACK_SEEN_REMOVE);
	}
	else
	{
		snprintf(out, outsize, "%u ok", CMDSTATUS_SUCCESS);
		mod->update_seen_questions(mod, STACK_SEEN_ADD);
	}
	mod->frontend->methods.clear(mod->frontend);

	return DC_OK;
}

int
command_get(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	struct question *q;
	char *argv[3];
	int argc;

	argc = strcmdsplit(arg, argv, DIM(argv));
	CHECKARGC(== 1);

	q = mod->questions->methods.get(mod->questions, argv[0]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s doesn't exist",
			CMDSTATUS_BADQUESTION, argv[0]);
	else 
	{
		const char *value = question_getvalue(q, NULL);
		snprintf(out, outsize, "%u %s",
			CMDSTATUS_SUCCESS, value ? value : "");
	}
	question_deref(q);

	return DC_OK;
}

int
command_set(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    struct question *q;
    char *argv[2] = { "", "" };
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(>= 1);

    q = mod->questions->methods.get(mod->questions, argv[0]);
    if (q == NULL)
        snprintf(out, outsize, "%u %s doesn't exist",
                CMDSTATUS_BADQUESTION, argv[0]);
    else
    {
        question_setvalue(q, argv[1]);

        if (mod->questions->methods.set(mod->questions, q) != 0)
        {
            snprintf(out, outsize, "%u value set",
                    CMDSTATUS_SUCCESS);
            if (0 == strcmp("debconf/language", argv[0]))
	    { /* Pass the value on to getlanguage() in templates.c */
                setenv("LANGUAGE", argv[1], 1);
	    }
        }
        else
            snprintf(out, outsize, "%u cannot set value",
                    CMDSTATUS_INTERNALERROR);
    }
    question_deref(q);

    return DC_OK;
}

int
command_reset(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	struct question *q;
	char *argv[2];
	int argc;

	argc = strcmdsplit(arg, argv, DIM(argv));
	CHECKARGC(== 1);

	q = mod->questions->methods.get(mod->questions, argv[0]);
	if (q == NULL)
		snprintf(out, outsize, "%u %s doesn't exist",
			CMDSTATUS_BADQUESTION, argv[0]);
	else
	{
		DELETE(q->value);
		q->flags &= ~DC_QFLAG_SEEN;

		if (mod->questions->methods.set(mod->questions, q) != 0)
			snprintf(out, outsize, "%u", CMDSTATUS_SUCCESS);
		else
			snprintf(out, outsize, "%u cannot reset value",
				CMDSTATUS_INTERNALERROR);
	}
	question_deref(q);

	return DC_OK;
}

int
command_subst(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    struct question *q;
    struct questionvariable;
    char *variable;
    char *argv[3] = { "", "", "" };
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(>= 2);

    variable = argv[1];
    q = mod->questions->methods.get(mod->questions, argv[0]);

    if (q == NULL)
        snprintf(out, outsize, "%u %s doesn't exist",
                CMDSTATUS_BADQUESTION, argv[0]);
    else
    {
        question_variable_add(q, variable, argv[2]);

        if (mod->questions->methods.set(mod->questions, q) != 0)
            snprintf(out, outsize, "%u", CMDSTATUS_SUCCESS);
        else
            snprintf(out, outsize, "%u substitution failed",
                    CMDSTATUS_INTERNALERROR);
    }
    question_deref(q);

    return DC_OK;
}

int
command_register(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    struct question *q;
    struct template *t;
    char *argv[4];
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(== 2);

    t = mod->templates->methods.get(mod->templates, argv[0]);
    if (t == NULL) {
        snprintf(out, outsize, "%u No such template, \"%s\"",
                CMDSTATUS_BADQUESTION, argv[0]);
        return DC_OK;
    }
    q = mod->questions->methods.get(mod->questions, argv[1]);
    if (q == NULL)
        q = question_new(argv[1]);
    if (q == NULL) {
        snprintf(out, outsize, "%u Internal error making question",
                CMDSTATUS_INTERNALERROR);
        return DC_OK;
    }
    question_owner_add(q, mod->owner);
    q->template = t;
    template_ref(t);
    mod->questions->methods.set(mod->questions, q);
    snprintf(out, outsize, "%u", CMDSTATUS_SUCCESS);
    return DC_OK;
}

int
command_unregister(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    struct question *q;
    char *argv[3];
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(== 1);

    q = mod->questions->methods.get(mod->questions, argv[0]);
    if (q == NULL) {
        snprintf(out, outsize, "%u %s doesn't exist",
                CMDSTATUS_BADQUESTION, argv[0]);
        return DC_OK;
    }
    question_owner_delete(q, mod->owner);
    snprintf(out, outsize, "%u", CMDSTATUS_SUCCESS);

    return DC_OK;
}

int
command_purge(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	mod->questions->methods.disownall(mod->questions, mod->owner);
	snprintf(out, outsize, "%u", CMDSTATUS_SUCCESS);
	return DC_OK;
}

int
command_metaget(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    struct question *q;
    const char *value;
    char *argv[4];
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(== 2);
    q = mod->questions->methods.get(mod->questions, argv[0]);
    if (q == NULL)
    {
        snprintf(out, outsize, "%u %s doesn't exist",
                CMDSTATUS_BADQUESTION, argv[0]);
        return DC_OK;
    }

    value = question_get_field(q, NULL, argv[1]);
    if (value == NULL)
        snprintf(out, outsize, "%u %s does not exist",
                CMDSTATUS_BADQUESTION, argv[1]);
    else
        snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS, value);

    return DC_OK;
}

int
command_fget(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    struct question *q;
    char *field;
    char *argv[4];
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(== 2);
    q = mod->questions->methods.get(mod->questions, argv[0]);
    if (q == NULL)
    {
        snprintf(out, outsize, "%u %s doesn't exist",
                CMDSTATUS_BADQUESTION, argv[0]);
        return DC_OK;
    }

    field = argv[1];
    /* isdefault is for backward compability only */
    if (strcmp(field, "seen") == 0)
        snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS,
                ((q->flags & DC_QFLAG_SEEN) ? "true" : "false"));
    else if (strcmp(field, "isdefault") == 0)
        snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS,
                ((q->flags & DC_QFLAG_SEEN) ? "false" : "true"));
    else
        snprintf(out, outsize, "%u %s does not exist", CMDSTATUS_BADPARAM, field);
    question_deref(q);

    return DC_OK;
}

int
command_fset(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    struct question *q;
    char *field;
    char *argv[5];
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(== 3);
    q = mod->questions->methods.get(mod->questions, argv[0]);
    if (q == NULL)
    {
        snprintf(out, outsize, "%u %s doesn't exist",
                CMDSTATUS_BADQUESTION, argv[0]);
        return DC_OK;
    }

    field = argv[1];
    if (strcmp(field, "seen") == 0)
    {
        q->flags &= ~DC_QFLAG_SEEN;
        if (strcmp(argv[2], "true") == 0)
            q->flags |= DC_QFLAG_SEEN;
    }
    else if (strcmp(field, "isdefault") == 0)
    {
        q->flags &= ~DC_QFLAG_SEEN;
        if (strcmp(argv[2], "false") == 0)
            q->flags |= DC_QFLAG_SEEN;
    }
    snprintf(out, outsize, "%u %s", CMDSTATUS_SUCCESS, argv[2]);

    question_deref(q);
    return DC_OK;
}

int
command_exist(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    struct question *q;
    char *argv[3];
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(== 1);

    q = mod->questions->methods.get(mod->questions, argv[0]);
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

int
command_stop(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
	char *argv[3];
	int argc;

	argc = strcmdsplit(arg, argv, DIM(argv));
	CHECKARGC(== 0);
	return DC_OK;
}

int
command_x_loadtemplatefile(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    struct template *t = NULL;
    struct question *q = NULL;
    char *argv[3];
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(== 1);
    t = template_load(argv[0]);
    while (t)
    {
        mod->templates->methods.set(mod->templates, t);
        q = mod->questions->methods.get(mod->questions, t->tag);
        if (q == NULL) {
            q = question_new(t->tag);
            q->template = t;
            template_ref(t);
        }
        mod->questions->methods.set(mod->questions, q);
        t = t->next;
    }
    snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
    return DC_OK;
}

int
command_progress(struct confmodule *mod, char *arg, char *out, size_t outsize)
{
    int min, max;
    struct question *q = NULL;
    const char *value;
    char *argv[6];
    int argc;

    argc = strcmdsplit(arg, argv, DIM(argv));
    CHECKARGC(>= 1);

    if (strcasecmp(argv[0], "start") == 0)
    {
        CHECKARGC(== 4);

        min = atoi(argv[1]);
        max = atoi(argv[2]);

        if (min > max)
        {
		    snprintf(out, outsize, "%u min (%d) > max (%d)", 
			    CMDSTATUS_SYNTAXERROR, min, max);
            return DC_NOTOK;
        }

        q = mod->questions->methods.get(mod->questions, argv[3]);
        if (q == NULL)
        {
            snprintf(out, outsize, "%u %s does not exist",
                    CMDSTATUS_BADQUESTION, argv[3]);
            return DC_NOTOK;
        }
        value = question_get_field(q, "", "description");
        if (value == NULL)
        {
            snprintf(out, outsize, "%u %s description field does not exist",
                    CMDSTATUS_BADQUESTION, argv[3]);
            return DC_NOTOK;
        }
        mod->frontend->methods.progress_start(mod->frontend,
            min, max, value);
    }
    else if (strcasecmp(argv[0], "set") == 0)
    {
        CHECKARGC(== 2);
        mod->frontend->methods.progress_set(mod->frontend, atoi(argv[1]));
    }
    else if (strcasecmp(argv[0], "step") == 0)
    {
        CHECKARGC(== 2);
        mod->frontend->methods.progress_step(mod->frontend, atoi(argv[1]));
    }
    else if (strcasecmp(argv[0], "info") == 0)
    {
        CHECKARGC(== 2);
        q = mod->questions->methods.get(mod->questions, argv[1]);
        if (q == NULL)
        {
            snprintf(out, outsize, "%u %s does not exist",
                    CMDSTATUS_BADQUESTION, argv[1]);
            return DC_NOTOK;
        }
        value = question_get_field(q, "", "description");
        if (value == NULL)
        {
            snprintf(out, outsize, "%u %s description field does not exist",
                    CMDSTATUS_BADQUESTION, argv[1]);
            return DC_NOTOK;
        }
        mod->frontend->methods.progress_info(mod->frontend, value);
    }
    else if (strcasecmp(argv[0], "stop") == 0)
    {
        mod->frontend->methods.progress_stop(mod->frontend);
    }

    snprintf(out, outsize, "%u OK", CMDSTATUS_SUCCESS);
    return DC_OK;
}

