#include "common.h"
#include "template.h"
#include "question.h"
#include "frontend.h"
#include "database.h"

#include <string.h>

struct uidata {
	int foo;
};
#define UIDATA(obj) ((struct uidata *)(obj)->data)

static void texthandler_displaydesc(struct frontend *obj, struct question *q) {
	int i;
	printf("%s\n", obj->title);
	for (i = 0; i < strlen(obj->title); i++) {
		printf("=");
	}
	printf("\n\n");
	printf("%s", q->template->extended_description);
	printf("\n");
	printf("%s\n", q->template->description);
}

static int texthandler_boolean(struct frontend *obj, struct question *q)
{
	char buf[30];
	int ans = -1;
	int def = -1;
	texthandler_displaydesc(obj, q);
	if (q->defaultval)
	{
		if (strcmp(q->defaultval, "true") == 0)
			def = 1;
		else if (strcmp(q->defaultval, "false") == 0)
			def = 0;
	}

	do {
		printf("%s / %s> ", (def == 1 ? "[yes]" : "yes"),
			(def == 0 ? "[no]" : "no"));

		fgets(buf, sizeof(buf), stdin);
		if (strcmp(buf, "yes\n") == 0)
			ans = 1;
		else if (strcmp(buf, "no\n") == 0)
			ans = 0;
		else if (q->defaultval && strcmp(buf, "\n") == 0)
			ans = def;
	} while (ans < 0);

	question_setvalue(q, (ans ? "true" : "false"));
	return DC_OK;
}

static int texthandler_multiselect(struct frontend *obj, struct question *q)
{
	texthandler_displaydesc(obj, q);
	return 0;
}

static int texthandler_note(struct frontend *obj, struct question *q)
{
	texthandler_displaydesc(obj, q);
	return 0;
}

static int texthandler_password(struct frontend *obj, struct question *q)
{
	texthandler_displaydesc(obj, q);
	return 0;
}

static int texthandler_select(struct frontend *obj, struct question *q)
{
	texthandler_displaydesc(obj, q);
	return 0;
}

static int texthandler_string(struct frontend *obj, struct question *q)
{
	texthandler_displaydesc(obj, q);
	return 0;
}

static int texthandler_text(struct frontend *obj, struct question *q)
{
	texthandler_displaydesc(obj, q);
	return 0;
}

/* ----------------------------------------------------------------------- */
struct question_handlers {
	const char *type;
	int (*handler)(struct frontend *obj, struct question *q);
} question_handlers[] = {
	{ "boolean",	texthandler_boolean },
	{ "multiselect", texthandler_multiselect },
	{ "note",	texthandler_note },
	{ "password",	texthandler_password },
	{ "select",	texthandler_select },
	{ "string",	texthandler_string },
	{ "text",	texthandler_text }
};

static int text_initialize(struct frontend *obj, struct configuration *conf)
{
	obj->interactive = 1;
	return DC_OK;
}

static int text_go(struct frontend *obj)
{
	struct question *q = obj->questions;
	int i;
	int ret;

	for (; q != 0; q = q->next)
	{
		for (i = 0; i < sizeof(question_handlers) / sizeof(question_handlers[0]); i++)
			if (strcmp(q->template->type, question_handlers[i].type) == 0)
			{
				ret = question_handlers[i].handler(obj, q);
				if (ret == DC_OK)
					obj->db->question_set(obj->db, q);
				return ret;
			}
	}

	return 0;
}

struct frontend_module debconf_frontend_module =
{
	initialize: text_initialize,
	go: text_go,
};
