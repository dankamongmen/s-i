/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: slang.c
 *
 * Description: SLang-based cdebconf UI module
 *
 * $Id: slang.c,v 1.8 2001/01/21 01:12:40 tausq Rel $
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
#include "debug.h"
#include "template.h"
#include "question.h"
#include "frontend.h"
#include "database.h"
#include "slang.h"
#include "strutl.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define CONFIGPREFIX	"frontend::driver::slang::"
#define WIN_QUERY	1
#define WIN_DESC	2
#define LINES		(SLtt_Screen_Rows ? SLtt_Screen_Rows : 24)
#define COLS		(SLtt_Screen_Cols ? SLtt_Screen_Cols : 80)
#define UIDATA(obj) 	((struct uidata *)(obj)->data)

#ifndef _
#define _(x) x
#endif

/* Private variables */
struct uidata {
	struct slwindow qrywin, descwin;
	int descstart;
};

/*
 * slang_flush
 *
 * flushes all pending draw commands to the screen, and moves the cursor to
 * the bottom right corner
 */
static void slang_flush(void)
{
	SLsmg_gotorc(SLtt_Screen_Rows-1, SLtt_Screen_Cols-1);
	SLsmg_refresh();
}

/*
 * slang_setcolor
 *
 * sets the color of a given object from the configuration
 */
static void slang_setcolor(struct frontend *ui, int *handle, const char *obj, 
	const char *defaultfg, const char *defaultbg)
{
	static int colorsused = 0;
	char fgname[50], bgname[50];

	snprintf(fgname, sizeof(fgname), CONFIGPREFIX "%s_fg", obj);
	snprintf(bgname, sizeof(bgname), CONFIGPREFIX "%s_bg", obj);
	*handle = colorsused++;
	SLtt_set_color(*handle, 0, (char *)ui->config->get(ui->config, fgname, 
		defaultfg), (char *)ui->config->get(ui->config, bgname, 
		defaultbg));
}

/*
 * slang_initwin
 *
 * initializes the various "windows" -- setup positions, colors, etc
 */
static void slang_initwin(struct frontend *ui, int type, struct slwindow *win)
{
	switch (type)
	{
	case WIN_QUERY:
		win->border = 1;
		win->x = win->y = 0;
		win->w = COLS;
		win->h = LINES*2/3;
		slang_setcolor(ui, &win->drawcolor, "query::draw", "yellow",
			"blue");
		slang_setcolor(ui, &win->fillcolor, "query::fill", "blue",
			"blue");
		slang_setcolor(ui, &win->selectedcolor, "query::selected",
			"black", "yellow");
		break;
	case WIN_DESC:
		win->border = 1;
		win->x = 0;
		win->y = LINES*2/3;
		win->w = COLS;
		win->h = LINES - win->y;
		slang_setcolor(ui, &win->drawcolor, "desc:::draw", "yellow",
			"blue");
		slang_setcolor(ui, &win->fillcolor, "desc::fill", "blue",
			"blue");
		slang_setcolor(ui, &win->selectedcolor, "desc::selected",
			"black", "yellow");
		break;
	default:
		INFO(INFO_DEBUG, _("Unrecognized window type"));
	}
}

/*
 * slang_drawwin
 *
 * Draws a window onto the screen; if the window has a border, draw that too
 */
static void slang_drawwin(const struct slwindow *win)
{
	SLsmg_set_color(win->fillcolor);
	SLsmg_fill_region(win->y, win->x, win->h, win->w, ' ');

	SLsmg_set_color(win->drawcolor);
	if (win->border > 0)
		SLsmg_draw_box(win->y, win->x, win->h, win->w);
}

/*
 * slang_wrapprint
 *
 * print a string to a window with linewrap
 */
static void slang_wrapprint(struct slwindow *win, const char *str, int start)
{
	int i, lc;
	char *lines[500];

	lc = strwrap(str, win->w - win->border * 2 - 1, lines, DIM(lines));

	SLsmg_set_color(win->drawcolor);
	for (i = start; i < lc && (i - start) < (win->h - win->border * 2); i++)
	{
		SLsmg_gotorc(win->y + win->border + i - start, win->x + win->border);
		SLsmg_write_string(lines[i]);
		DELETE(lines[i]);
	}

	if (start > 0)
	{
		SLsmg_gotorc(win->y + win->border, win->x + win->w - 1);
		SLsmg_write_char('^');
	}
	if (i < lc)
	{
		SLsmg_gotorc(win->y + win->h - 2, win->x + win->w - 1);
		SLsmg_write_char('v');
	}
}

/*
 * slang_drawdesc
 *
 * draws the description for a question onto the screen
 */
static void slang_drawdesc(struct frontend *ui, struct question *q)
{
	struct uidata *uid = UIDATA(ui);

	/* Clear the windows */
	slang_drawwin(&uid->qrywin);
	slang_drawwin(&uid->descwin);
	/* Draw in the descriptions */
	slang_wrapprint(&uid->qrywin, question_description(q), 0);
	slang_wrapprint(&uid->descwin, question_extended_description(q),
		uid->descstart);

	/* caller should call slang_flush() ! */
}

/*
 * slang_printf
 *
 * prints the given string at a specific screen coordinate and color
 */
static void slang_printf(int r, int c, int col, char *fmt, ...)
{
	va_list ap;

	SLsmg_gotorc(r, c);
	SLsmg_set_color(col);
	va_start(ap, fmt);
	SLsmg_vprintf(fmt, ap);
	va_end(ap);
}

/*
 * slang_keyhandler
 *
 * common portion of a keyboard handler for each of the widgets; handles
 * scrolling between the navigation buttons and other active widgets. When
 * the user presses ENTER the currently active element is returned, otherwise
 * returns -1
 */
static int slang_keyhandler(struct frontend *ui, struct question *q, int *pos, 
	int maxpos, int ch)
{
	struct uidata *uid = UIDATA(ui);

	if (ch == 0) ch = SLkp_getkey();
	switch (ch)
	{
	case SL_KEY_PPAGE:
		uid->descstart -= (uid->descwin.h / 2);
		if (uid->descstart < 0) uid->descstart = 0;
		slang_drawdesc(ui, q);
		break;
	case SL_KEY_NPAGE:
		uid->descstart += (uid->descwin.h / 2);
		slang_drawdesc(ui, q);
		break;
	case SL_KEY_LEFT:
	case SL_KEY_UP:
		(*pos)--;
		if (*pos < 0) *pos = maxpos;
		break;
	case SL_KEY_RIGHT:
	case SL_KEY_DOWN:
	case 9: /* tab */
		(*pos)++;
		if (*pos > maxpos) *pos = 0;
		break;
	case ' ':
	case '\r':
	case '\n':
	case SL_KEY_ENTER:
		return *pos;
		break;
	}

	return -1;
}

/*
 * slang_navbuttons
 *
 * draws the navigation buttons (previous, next) onto the screen
 *
 * TODO: if cangoback/forward is false, draw the buttons in a "disabled"
 * color
 */
static void slang_navbuttons(struct frontend *ui, struct question *q, 
	int selected)
{
	struct uidata *uid = UIDATA(ui);
	struct slwindow *win = &uid->qrywin;
	int ybut = win->y + win->h - win->border;

	SLsmg_set_color(win->drawcolor);
	SLsmg_gotorc(ybut-2, 1);
	SLsmg_draw_hline(COLS-2);
	SLsmg_set_char_set(1);
	SLsmg_gotorc(ybut-2, 0);
	SLsmg_write_char(SLSMG_LTEE_CHAR);
	SLsmg_gotorc(ybut-2, COLS-1);
	SLsmg_write_char(SLSMG_RTEE_CHAR);
	SLsmg_set_char_set(0);

	/* draw the actual buttons, note that these are drawn in the
	 * query window instead of in the parent (like the frame) */

	if (ui->cangoback(ui, q))
	{
		slang_printf(ybut - 1, 2, (selected == 0 ? win->selectedcolor :
			win->drawcolor), _(" <Previous> "));
	}

	if (ui->cangoforward(ui, q))
	{
		slang_printf(ybut - 1, COLS-10, (selected == 1 ? 
			win->selectedcolor : win->drawcolor), _(" <Next> "));
	}

	/* caller should call slang_flush() ! */
}

/* --------------------------------------------------------------------- */
static int slang_boolean(struct frontend *ui, struct question *q)
{
	struct uidata *uid = UIDATA(ui);
	struct slwindow *win = &uid->qrywin;
	int ybut = win->y + win->h - win->border - 4;
	const char *value = "true";
	int ret = 0, ans, pos = 2;

	value = question_defaultval(q);

	ans = (strcmp(value, "true") == 0);

	while (ret == 0) 
	{
		/* Draw the radio boxes */
		slang_printf(ybut, (COLS/2)-11, (pos == 2 ? win->selectedcolor :
			win->drawcolor), " (%c) %s ", (ans ? '*' : ' '), 
			_("Yes"));
		slang_printf(ybut, (COLS/2)+4, (pos == 3 ? win->selectedcolor :
			win->drawcolor), " (%c) %s ", (ans ? ' ' : '*'),
			_("No"));

		slang_navbuttons(ui, q, pos);

		slang_flush();

		switch (slang_keyhandler(ui, q, &pos, 3, 0))
		{
		case 0: ret = DC_GOBACK; break;
		case 1: ret = DC_OK; break;
		case 2: ans = 1; break;
		case 3: ans = 0; break;
		}
	}

	if (ret == DC_OK)
		question_setvalue(q, (ans ? "true" : "false"));
	
	return ret;
}

static int slang_note(struct frontend *ui, struct question *q)
{
	int ret = 0, pos = 0;

	while (ret == 0)
	{
		slang_navbuttons(ui, q, pos);
		slang_flush();

		switch (slang_keyhandler(ui, q, &pos, 1, 0))
		{
		case 0: ret = DC_GOBACK; break;
		case 1: ret = DC_OK; break;
		}
	}
	return ret;
}

static int slang_getselect(struct frontend *ui, struct question *q, int multi)
{
	char *choices[100] = {0};
	char *defaults[100] = {0};
	char selected[100] = {0};
	char answer[1024] = {0};
	int i, j, count, dcount, ret = 0, val = 0, pos = 2, xpos, ypos;
	int top, bottom, longest, ch;
	struct uidata *uid = UIDATA(ui);
	struct slwindow *win = &uid->qrywin;

	/* Parse out all the choices */
	count = strchoicesplit(question_choices(q), choices, DIM(choices));
	dcount = strchoicesplit(question_defaultval(q), defaults, DIM(defaults));
	INFO(INFO_VERBOSE, "Parsed out %d choices, %d defaults\n", count, dcount);
	if (count <= 0) return DC_NOTOK;

	/* See what the currently selected value should be -- either a
	 * previously selected value, or the default for the question
	 */
	for (j = 0; j < dcount; j++)
	{
		for (i = 0; i < count; i++)
			if (strcmp(choices[i], defaults[j]) == 0)
				selected[i] = 1;
	}

	longest = strlongest(choices, count);
	top = 0;
	if (top < 0) top = 0;
	xpos = (COLS-longest)/2-1;

	while (ret == 0)
	{
		ypos = 3;
		bottom = top + MIN(count, win->h - 7);

		INFO(INFO_VERBOSE, "[val; %d, top: %d, bottom: %d]\n", val,
			top, bottom);
		for (i = top; i < bottom; i++)
		{
			slang_printf(ypos++, xpos, ((pos == 2 && i == val) ?
				win->selectedcolor : win->drawcolor),
				"(%c) %-*s ", (selected[i] ? '*' : ' '), 
				longest, choices[i]);

			INFO(INFO_VERBOSE, "(%c) %-*s\n", (selected[i] ? '*' : 
				' '), longest, choices[i]);
		}

		slang_navbuttons(ui, q, pos);
		slang_flush();

		switch ((ch = SLkp_getkey()))
		{
		case SL_KEY_LEFT:
		case SL_KEY_UP:
			val--;
			if (val < 0)
			{
				val = count-1;
				top = val - win->h + 8;
				if (top < 0) top = 0;
			}
			if (val < top) top = val;
			/* TODO: check val against top/bottom */
			break;
		case SL_KEY_RIGHT:
		case SL_KEY_DOWN:
			val++;
			if (val >= count)
			{
				val = 0;
				top = 0;
			}
			if (val >= bottom) top++;
			/* TODO: check val against top/bottom */
			break;
		default:
			switch (slang_keyhandler(ui, q, &pos, 2, ch))
			{
			case 0: ret = DC_GOBACK; break;
			case 1: ret = DC_OK; break;
			default: 
				if (multi == 0)
				{
					memset(selected, 0, sizeof(selected));
					selected[val] = 1;
				}
				else
				{
					selected[val] = !selected[val];
				}
			}
		}
	}
	if (ret != DC_OK) return ret;

	for (i = 0; i < count; i++)
	{
		if (selected[i])
		{
			if (answer[0] != 0)
				strvacat(answer, sizeof(answer), ", ");
			strvacat(answer, sizeof(answer), choices[i], NULL);
		}
		free(choices[i]);
	}
	for (i = 0; i < dcount; i++)
		free(defaults[i]);
	question_setvalue(q, answer);
	return DC_OK;
}

static int slang_select(struct frontend *ui, struct question *q)
{
	INFO(INFO_VERBOSE, "calling getselect\n");
	return slang_getselect(ui, q, 0);
}

static int slang_multiselect(struct frontend *ui, struct question *q)
{
	return slang_getselect(ui, q, 1);
}

static int slang_getstring(struct frontend *ui, struct question *q, char showch)
{
	char value[1024] = {0};
	int ret = 0, pos = 2, ch;
	struct uidata *uid = UIDATA(ui);
	struct slwindow *win = &uid->qrywin;
	int xpos = 0, ypos = win->y + win->border + 4;
	int cursor;
	char *tmp;

	STRCPY(value, question_defaultval(q));
	cursor = strlen(value);

	/* TODO: scrolling */
	while (ret == 0)
	{
		if (showch == 0)
		{
			slang_printf(ypos, 2, win->selectedcolor, "%-*s", 
				win->w - 5, value);
		}
		else
		{
			slang_printf(ypos, 2, win->selectedcolor, "%-*s", 
				win->w - 5, " ");
			for (xpos = 0; xpos < strlen(value); xpos++)
			{
				SLsmg_gotorc(ypos, xpos+2);
				SLsmg_write_char(showch);
			}
		}

		slang_navbuttons(ui, q, pos);
		slang_flush();

		if (pos == 2)
		{
			SLsmg_gotorc(ypos, 2+cursor);
			SLsmg_refresh();
		}

		ch = SLkp_getkey();

		if (isprint(ch))
		{	
			value[cursor] = (char)ch;
			if (cursor < sizeof(value))
				cursor++;
		}
		else
		{
			switch (ch)
			{
			case SL_KEY_HOME:
			case 1: /* ^A */
				cursor = 0;
				break;
			case SL_KEY_END:
			case 5: /* ^E */
				cursor = strlen(value);
				break;
			case 21: /* ^U */
				memset(value, 0, sizeof(value));
				cursor = 0;
				break;
			case SL_KEY_BACKSPACE:
				/* TODO */
				if (cursor > 0)
				{
					tmp = &value[cursor];
					do {
						*(tmp-1) = *tmp;
					} while (*tmp++ != 0);
					cursor--;
				}
				break;
			case SL_KEY_LEFT:
				if (cursor > 0) cursor--; 
				break;
			case SL_KEY_RIGHT:
				if (cursor < strlen(value)) cursor++;
				break;
			default:
				switch (slang_keyhandler(ui, q, &pos, 2, ch))
				{
				case 0: ret = DC_GOBACK; break;
				case 1: ret = DC_OK; break;
				}
			}
		}
	}	 

	if (ret == DC_OK)
		question_setvalue(q, value);
	return ret;
}

static int slang_string(struct frontend *ui, struct question *q)
{
	return slang_getstring(ui, q, 0);
}

static int slang_password(struct frontend *ui, struct question *q)
{
	return slang_getstring(ui, q, '*');
}

static int slang_text(struct frontend *ui, struct question *q)
{
	return 0;
}


/* ----------------------------------------------------------------------- */
static struct question_handlers {
	const char *type;
	int (*handler)(struct frontend *, struct question *q);
} question_handlers[] = {
	{ "boolean",	slang_boolean },
	{ "multiselect", slang_multiselect },
	{ "note",	slang_note },
	{ "password",	slang_password },
	{ "select",	slang_select },
	{ "string",	slang_string },
	{ "text",	slang_text }
};

static int slang_initialize(struct frontend *obj, struct configuration *cfg)
{
	struct uidata *uid = NEW(struct uidata);
	int ret;

	memset(uid, 0, sizeof(struct uidata));
	obj->interactive = 1;
	obj->data = uid;

	SLtt_get_terminfo();
	ret = SLsmg_init_smg();
	INFO(INFO_DEBUG, "SLsmg_init_smg returned %d\n", ret);
	ret = SLang_init_tty(-1, 0, 0);
	INFO(INFO_DEBUG, "SLang_init_tty returned %d\n", ret);
	ret = SLkp_init();
	INFO(INFO_DEBUG, "SLkp_init returned %d\n", ret);

	slang_initwin(obj, WIN_QUERY, &uid->qrywin);
	slang_initwin(obj, WIN_DESC, &uid->descwin);

	slang_drawwin(&uid->qrywin);
	slang_drawwin(&uid->descwin);
	slang_flush();
	
	return DC_OK;
}

static int slang_shutdown(struct frontend *obj)
{
	struct uidata *uid = UIDATA(obj);

	SLsmg_reset_smg();
	SLang_reset_tty();

	DELETE(uid);
	return DC_OK;
}

static int slang_go(struct frontend *obj)
{
	struct question *q = obj->questions;
	int i;
	int ret;

	while (q != 0)
	{
		ret = DC_OK;
		for (i = 0; i < DIM(question_handlers); i++)
		{
			INFO(INFO_VERBOSE, "For question [%s], comparing question type [%s] against handler %d (%s)\n", q->tag, q->template->type, i, question_handlers[i].type);
			if (strcasecmp(q->template->type, question_handlers[i].type) == 0)
			{
				INFO(INFO_VERBOSE, "Found one!\n");
				UIDATA(obj)->descstart = 0;
				slang_drawdesc(obj, q);
				ret = question_handlers[i].handler(obj, q);
				INFO(INFO_VERBOSE, "Return code = %d\n", ret);
				switch (ret)
				{
				case DC_OK:
					obj->db->question_set(obj->db, q);
					break;
				case DC_GOBACK:
					if (q->prev != NULL)
					{
						q = q->prev;
						break;
					}
					/* fallthrough */
				default:
					return ret;
				}
				break;
			}
		}
		if (ret == DC_OK)
			q = q->next;
	}

	return DC_OK;
}

struct frontend_module debconf_frontend_module =
{
	initialize: slang_initialize,
	shutdown: slang_shutdown,
	go: slang_go,
};
