#include "common.h"
#include "question.h"
#include "template.h"
#include "strutl.h"

struct question *question_new(const char *tag)
{
	struct question *q;

	q = NEW(struct question);
	memset(q, 0, sizeof(struct question));
	q->ref = 1;
	q->tag = STRDUP(tag);

	return q;
}

void question_delete(struct question *question)
{
	DELETE(question);
}

void question_ref(struct question *q)
{
	++q->ref;
}

void question_deref(struct question *q)
{
	if (--q->ref == 0)
		question_delete(q);
}

void question_setvalue(struct question *q, const char *value)
{
	free(q->value);
	q->value = STRDUP(value);
}

void question_variable_add(struct question *q, const char *var, 	
	const char *value)
{
	struct questionvariable *qvi = q->variables;
	struct questionvariable **qlast = &q->variables;

	for (; qvi != 0; qlast = &qvi->next, qvi = qvi->next)
		if (strcmp(qvi->variable, var) == 0)
		{
			DELETE(qvi->value);
			qvi->value = STRDUP(value);
			return;
		}
	
	qvi = NEW(struct questionvariable);
	memset(qvi, 0, sizeof(struct questionvariable));
	qvi->variable = STRDUP(var);
	qvi->value = STRDUP(value);
	*qlast = qvi;
}

void question_owner_add(struct question *q, const char *owner)
{
	struct questionowner **ownerp = &q->owners;
	
	while (*ownerp != 0)
	{
		if (strcmp((*ownerp)->owner, owner) == 0)
			return;
		ownerp = &(*ownerp)->next;
	}

	*ownerp = NEW(struct questionowner);
	(*ownerp)->owner = STRDUP(owner);
	(*ownerp)->next = 0;
}

void question_owner_delete(struct question *q, const char *owner)
{
	struct questionowner **ownerp, *nextp;

	for (ownerp = &q->owners; *ownerp != 0; ownerp = &(*ownerp)->next)
	{
		if (strcmp((*ownerp)->owner, owner) == 0)
		{
			nextp = (*ownerp)->next;
			if (nextp == 0)
			{
				nextp = *ownerp;
				*ownerp = 0;
			}
			else
			{
				**ownerp = *nextp;
			}
			DELETE(nextp->owner);
			DELETE(nextp);
		}
	}
}

static int question_expand_vars(struct question *q, const char *field, 
	char *buf, size_t maxlen)
{
	int i = 0;
	const char *p = field, *varend;
	char var[100];
	struct questionvariable *qvi = q->variables;

	memset(buf, 0, maxlen);
	if (p == 0) return 0;

	while (*p != 0 && i < maxlen - 1)
	{
		/* is this a variable string? */
		if (*p != '$' || *(p+1) != '{') 
		{
			buf[i++] = *p++;
			continue;
		}

		/* look for the end of the variable */
		varend = p + 2;
		while (*varend != 0 && *varend != '}') varend++;
		/* didn't find it? then don't consider this a variable */
		if (*varend == 0)
		{
			buf[i++] = *p++;
			continue;
		}

		strncpy(var, p+2, varend-(p+2));
		var[varend-(p+2)] = 0;

		for (; qvi != 0; qvi = qvi->next)
			if (strcmp(qvi->variable, var) == 0)
				break;

		if (qvi != 0)
		{
			strvacat(buf, maxlen, qvi->value, NULL);
			i = strlen(buf) - 1;
		}

		p = varend + 1;
	}
	return 1;
}

const char *question_description(struct question *q)
{
	static char buf[4096] = {0};
	question_expand_vars(q, q->template->description, buf, sizeof(buf));
	return buf;
}

const char *question_extended_description(struct question *q)
{
	static char buf[4096] = {0};
	question_expand_vars(q, q->template->extended_description, buf, sizeof(buf));
	return buf;
}

const char *question_choices(struct question *q)
{
	static char buf[4096] = {0};
	question_expand_vars(q, q->template->choices, buf, sizeof(buf));
	return buf;
}
