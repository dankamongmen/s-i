#include "common.h"
#include "template.h"
#include "question.h"
#include "frontend.h"
#include "database.h"
#include "ncurses.h"

#include <string.h>

#define QRYCOLOR	1
#define DESCCOLOR	2
#define UIDATA(obj) 	((struct uidata *)(obj)->data)

struct question_handlers {
	const char *type;
	int (*handler)(struct frontend *, struct question *q);
} question_handlers[] = {
	{ "boolean",	nchandler_boolean },
	{ "multiselect", nchandler_multiselect },
	{ "note",	nchandler_note },
	{ "password",	nchandler_password },
	{ "select",	nchandler_select },
	{ "string",	nchandler_string },
	{ "text",	nchandler_text }
};

/* Private variables */
struct uidata {
	int qrylines, desclines;
	WINDOW *qrywin, *descwin;
};

static int nchandler_boolean(struct frontend *ui, struct question *q)
{
	while(1);
	return DC_OK;
}

static int nchandler_multiselect(struct frontend *ui, struct question *q)
{
	return 0;
}

static int nchandler_note(struct frontend *ui, struct question *q)
{
	return 0;
}

static int nchandler_password(struct frontend *ui, struct question *q)
{
	return 0;
}

static int nchandler_select(struct frontend *ui, struct question *q)
{
	return 0;
}

static int nchandler_string(struct frontend *ui, struct question *q)
{
	return 0;
}

static int nchandler_text(struct frontend *ui, struct question *q)
{
	return 0;
}

/* ----------------------------------------------------------------------- */

static int ncurses_initialize(struct frontend *obj, struct configuration *cfg)
{
	struct uidata *uid = NEW(struct uidata);
	memset(uid, 0, sizeof(struct uidata));
	obj->interactive = 1;
	obj->data = uid;

	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	uid->qrylines = LINES / 3;
	uid->desclines = LINES - uid->qrylines;

	uid->qrywin = newwin(uid->qrylines, COLS, 0, 0);
	uid->descwin = newwin(uid->desclines - 1, COLS, uid->qrylines, 0);

	if (uid->qrywin == NULL || uid->descwin == NULL)
		return DC_NOTOK;

	init_pair(QRYCOLOR, COLOR_BLACK, COLOR_CYAN);
	init_pair(DESCCOLOR, COLOR_BLACK, COLOR_CYAN);

	wbkgdset(uid->qrywin, COLOR_PAIR(QRYCOLOR));
	wbkgdset(uid->descwin, COLOR_PAIR(DESCCOLOR));

	wclear(uid->qrywin);
	wclear(uid->descwin);

	wborder(uid->qrywin, 0, 0, 0, 0, 0, 0, 0, 0);
	wborder(uid->descwin, 0, 0, 0, 0, 0, 0, 0, 0);

	wrefresh(uid->qrywin);
	wrefresh(uid->descwin);

	return DC_OK;
}

static int ncurses_shutdown(struct frontend *obj)
{
	struct uidata *uid = UIDATA(obj);
	delwin(uid->qrywin);
	delwin(uid->descwin);
	endwin();
	DELETE(uid);
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
			if (strcasecmp(q->template->type, question_handlers[i].type) == 0)
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
	initialize: ncurses_initialize,
	shutdown: ncurses_shutdown,
	go: ncurses_go,
};
