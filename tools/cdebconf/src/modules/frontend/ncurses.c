#include "common.h"
#include "template.h"
#include "question.h"
#include "frontend.h"
#include "ncurses.h"

struct question_handlers {
	const char *type;
	int (*handler)(struct uidata *, struct question *q);
} question_handlers[] = {
	{ "boolean",	nchandler_boolean },
	{ "multiselect", nchandler_multiselect },
	{ "note",	nchandler_note },
	{ "password",	nchandler_password },
	{ "select",	nchandler_select },
	{ "string",	nchandler_string },
	{ "text",	nchandler_text }
};

static int nchandler_boolean(struct uidata *ui, struct question *q)
{
	return 0;
}

static int nchandler_multiselect(struct uidata *ui, struct question *q)
{
	return 0;
}

static int nchandler_note(struct uidata *ui, struct question *q)
{
	return 0;
}

static int nchandler_password(struct uidata *ui, struct question *q)
{
	return 0;
}

static int nchandler_select(struct uidata *ui, struct question *q)
{
	return 0;
}

static int nchandler_string(struct uidata *ui, struct question *q)
{
	return 0;
}

static int nchandler_text(struct uidata *ui, struct question *q)
{
	return 0;
}

/* ----------------------------------------------------------------------- */

static int ncurses_initialize(struct frontend *obj, struct configuration *cfg)
{
	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	return DC_OK;
}

static int ncurses_shutdown(struct frontend *obj)
{
	endwin();
	return DC_OK;
}

static int ncurses_go(struct frontend *obj)
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
	initialize: ncurses_initialize,
	shutdown: ncurses_shutdown,
	go: ncurses_go,
};
