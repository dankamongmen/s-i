#include "common.h"
#include "template.h"
#include "question.h"
#include "frontend.h"

struct uidata {
	int foo;
};

static void texthandler_displaydesc(struct uidata *ui, struct question *q) {
	printf("This is a question!\n");
	printf("===================\n");
	printf("\n");
	printf("%s", q->template->extended_description);
	printf("\n");
	printf("%s ...\n", q->template->description);
	printf("\n");
}

static int texthandler_boolean(struct uidata *ui, struct question *q)
{
	texthandler_displaydesc(ui, q);
	return 0;
}

static int texthandler_multiselect(struct uidata *ui, struct question *q)
{
	texthandler_displaydesc(ui, q);
	return 0;
}

static int texthandler_note(struct uidata *ui, struct question *q)
{
	texthandler_displaydesc(ui, q);
	return 0;
}

static int texthandler_password(struct uidata *ui, struct question *q)
{
	texthandler_displaydesc(ui, q);
	return 0;
}

static int texthandler_select(struct uidata *ui, struct question *q)
{
	texthandler_displaydesc(ui, q);
	return 0;
}

static int texthandler_string(struct uidata *ui, struct question *q)
{
	texthandler_displaydesc(ui, q);
	return 0;
}

static int texthandler_text(struct uidata *ui, struct question *q)
{
	texthandler_displaydesc(ui, q);
	return 0;
}

/* ----------------------------------------------------------------------- */
struct question_handlers {
	const char *type;
	int (*handler)(struct uidata *, struct question *q);
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
				ret = question_handlers[i].handler((struct uidata *)obj->data, q);
			}
		
	}

	return 0;
}

struct frontend_module debconf_frontend_module =
{
	initialize: text_initialize,
	go: text_go,
};
