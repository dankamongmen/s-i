/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: text.c
 *
 * Description: text UI for cdebconf
 * Some notes on the implementation - this is meant to be an accessibility-
 * friendly implementation. I've taken care to make the prompts work well
 * with screen readers and the like.
 *
 * $Id: text.c,v 1.37 2003/03/14 21:31:42 sjogren Exp $
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
#include "template.h"
#include "question.h"
#include "frontend.h"
#include "database.h"
#include "strutl.h"

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 * Function: getwidth
 * Input: none
 * Output: int - width of screen
 * Description: get the width of the current terminal
 * Assumptions: doesn't handle resizing; caches value on first call
 */
static const int getwidth(void)
{
	static int res = 80;
	static int inited = 0;
	int fd;
	struct winsize ws;

	if (inited == 0)
	{
		inited = 1;
		if ((fd = open("/dev/tty", O_RDONLY)) > 0)
		{
			if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
				res = ws.ws_col;
			close(fd);
		}
	}
	return res;
}

/*
 * Function: getheight
 * Input: none
 * Output: int - height of screen
 * Description: get the height of the current terminal
 * Assumptions: doesn't handle resizing; caches value on first call
 */
static const int getheight(void)
{
  static int res = 25;
  static int inited = 0;

  if (inited == 0) {
    int fd;

    inited = 1;
    if ((fd = open("/dev/tty", O_RDONLY)) > 0) {
      struct winsize ws;

      if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0)
	res = ws.ws_row;

      close(fd);
    }
  }

  return res;
}

/*
 * Function: wrap_print
 * Input: const char *str - string to display
 * Output: none
 * Description: prints a string to the screen with word wrapping 
 * Assumptions: string fits in <500 lines
 */
static void wrap_print(const char *str)
{
	/* Simple greedy line-wrapper */
	int i, lc;
	char *lines[500];

	lc = strwrap(str, getwidth() - 1, lines, DIM(lines));

	for (i = 0; i < lc; i++)
	{
		printf("%s\n", lines[i]);
		DELETE(lines[i]);
	}
}

/*
 * Function: texthandler_displaydesc
 * Input: struct frontend *obj - UI object
 *        struct question *q - question for which to display the description
 * Output: none
 * Description: displays the description for a given question 
 * Assumptions: none
 */
static void texthandler_displaydesc(struct frontend *obj, struct question *q) 
{
	wrap_print(question_get_field(q, "", "description"));
	wrap_print(question_get_field(q, "", "extended_description"));
}

/*
 * Function: texthandler_boolean
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK, DC_GOBACK
 * Description: handler for the boolean question type
 * Assumptions: none
 */
static int texthandler_boolean(struct frontend *obj, struct question *q)
{
	char buf[30];
	int ans = -1;
	int def = -1;
	const char *defval;

	defval = question_getvalue(q, "");
	if (defval)
	{
		if (strcmp(defval, "true") == 0)
			def = 1;
		else 
			def = 0;
	}

	/* turn of SIGINT before asking for input, otherwise things
	 * get very messy
	 */
	do {
		printf("%s%s> ", 
			obj->methods.can_go_back(obj, q) ? _("Prompt: yes/no/cancel") : _("Prompt: yes/no"), 
			(defval == NULL ? "" : (def == 0 ? _(", default=no") : _(", default=yes"))));

		fgets(buf, sizeof(buf), stdin);
		if ((strcmp (buf,_("cancel\n")) == 0) || (strcmp(buf,_("CANCEL")) == 0))
			return DC_GOBACK;
		if ((strcmp(buf, _("yes\n")) == 0) || (strcmp(buf, _("YES\n")) == 0))
			ans = 1;
		else if ((strcmp(buf, _("no\n")) == 0) || (strcmp(buf, _("NO\n")) == 0))
			ans = 0;
#if defined(__s390__) || defined (__s390x__)
		else if (defval && (strcmp(buf, "\n") == 0 || strcmp(buf, ".\n") == 0))
#else
		else if (defval && strcmp(buf, "\n") == 0)
#endif
			ans = def;
	} while (ans < 0);

	question_setvalue(q, (ans ? "true" : "false"));
	return DC_OK;
}

/*
 * Function: texthandler_multiselect
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the multiselect question type
 * Assumptions: none
 *
 * TODO: factor common code with select
 */
static int texthandler_multiselect(struct frontend *obj, struct question *q)
{
	char *choices[100] = {0};
	char *choices_translated[100] = {0};
	char *defaults[100] = {0};
	char selected[100] = {0};
	char answer[1024] = {0};
	int i, j, line, count, dcount, choice;

	count = strchoicesplit(question_get_field(q, NULL, "choices"), choices, DIM(choices));
	if (count <= 0) return DC_NOTOK;

	strchoicesplit(question_get_field(q, "", "choices"), choices_translated, DIM(choices_translated));
	dcount = strchoicesplit(question_get_field(q, NULL, "value"), defaults, DIM(defaults));

	for (j = 0; j < dcount; j++)
		for (i = 0; i < count; i++)
			if (strcmp(choices[i], defaults[j]) == 0)
				selected[i] = 1;

	i = 0;

	while (1) {
 	    for (line = 0; i < count && line < getheight()-1; i++, line++)
	        printf("%3d. %s%s\n", i+1, choices_translated[i], 
		       (selected[i] ? _(" (selected)") : ""));

	    if (i == count && count < getheight()-1) {
	        printf(_("Prompt: 1 - %d, q to end> "), count);
	    } else if (i == count) {
	        printf(_("Prompt: 1 - %d, q to end, b for begin> "), count);
	    } else {
	        printf(_("Prompt: 1 - %d/%d, q to end, n for next page> "), i, count);
	    }

	    fgets(answer, sizeof(answer), stdin);
	    if (answer[0] == 'q') break;
	    if (answer[0] == 'n') continue;
	    if (answer[0] == 'b') { i = 0; continue; }

	    choice = atoi(answer);

	    if (choice > 0 && choice <= count) {
	        if (selected[choice-1] == 0) 
	            selected[choice-1] = 1;
	        else
	            selected[choice-1] = 0;
	        i = choice-getheight()+1 > 0 ? choice-getheight()+1 : 0;
	    }
	}

	answer[0] = 0;
	for (i = 0; i < count; i++)
	{
		if (selected[i])
		{
			if (answer[0] != 0)
				strvacat(answer, sizeof(answer), ", ", NULL);
			strvacat(answer, sizeof(answer), choices[i], NULL);
		}
		free(choices[i]);
		free(choices_translated[i]);
	}
	for (i = 0; i < dcount; i++)
		free(defaults[i]);
	question_setvalue(q, answer);
	
	return DC_OK;
}

/*
 * Function: texthandler_note
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK, DC_GOBACK
 * Description: handler for the note question type
 * Assumptions: none
 */
static int texthandler_note(struct frontend *obj, struct question *q)
{
	int c;
	if (obj->methods.can_go_back (obj, q))
		printf (_("[Press enter to continue, or 'c to cancel]"));
	else
		printf(_("[Press enter to continue]\n"));
	do { 
		c = fgetc(stdin); 
		if ((obj->methods.can_go_back (obj, q)) &&  ((c == 'c' ) || (c == 'C')))
			return DC_GOBACK;
	} while (c != '\r' && c != '\n');
	return DC_OK;
}

/*
 * Function: texthandler_password
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the password question type
 * Assumptions: none
 *
 * TODO: this can be *MUCH* improved. no editing is possible right now
 */
static int texthandler_password(struct frontend *obj, struct question *q)
{
	struct termios oldt, newt;
	char passwd[256] = {0};
	int i = 0, c;

	tcgetattr(0, &oldt);
	memcpy(&newt, &oldt, sizeof(struct termios));
	cfmakeraw(&newt);
	tcsetattr(0, TCSANOW, &newt);
	while ((c = fgetc(stdin)) != EOF)
	{
		fputc('*', stdout);
		passwd[i++] = (char)c;
		if (c == '\r' || c == '\n') break;

	}
	printf("\n");
	passwd[i] = 0;
	tcsetattr(0, TCSANOW, &oldt);
	question_setvalue(q, passwd);
	return DC_OK;
}

/*
 * Function: texthandler_select
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the select question type
 * Assumptions: none
 *
 * TODO: factor common code with multiselect
 */
static int texthandler_select(struct frontend *obj, struct question *q)
{
	char *choices[100] = {0};
	char *choices_translated[100] = {0};
	char answer[10];
	int i, line, count, choice = 1, def = -1;
	const char *defval = question_getvalue(q, "");

	count = strchoicesplit(question_get_field(q, NULL, "choices"), choices, DIM(choices));
	if (count <= 0) return DC_NOTOK;
	if (count == 1)
		defval = choices[0];

	strchoicesplit(question_get_field(q, "", "choices"), choices_translated, DIM(choices_translated));
        /* fprintf(stderr,"In texthandler_select, count is: %d\n", count);*/
	if (defval != NULL)
	{
		for (i = 0; i < count; i++)
			if (strcmp(choices[i], defval) == 0)
				def = i + 1;
	}

	i = 0;

	do {
	    for (line = 0; i < count && line < getheight()-1; i++, line++)
	        printf("%3d. %s%s\n", i+1, choices_translated[i],
		       (def == i + 1 ? _(" (default)") : ""));
 
	    if (i == count) {
	        if (def > 0 && choices_translated[def-1]) {
	            printf(_("Prompt: 1 - %d, default=%s> "), count, choices_translated[def-1]);
	        } else {
	            printf(_("Prompt: 1 - %d> "), count);
                }
	    } else {
	        printf(_("Prompt: 1 - %d/%d, n for next page> "), i, count);
	    }
	    fgets(answer, sizeof(answer), stdin);
#if defined(__s390__) || defined (__s390x__)
	    if (answer[0] == '\n' || (answer[0] == '.' && answer[1] == '\n'))
#else
	    if (answer[0] == '\n')
#endif
	        choice = def;
	    else
	        choice = atoi(answer);
	} while (choice <= 0 || choice > count);
	/*	fprintf(stderr,"In %s, line: %d\n\tanswer: %s, choice[choice]: %s\n",
		__FILE__,__LINE__,answer, choices[choice - 1]);*/
	question_setvalue(q, choices[choice - 1]);
	for (i = 0; i < count; i++) 
	{
		free(choices[i]);
		free(choices_translated[i]);
	}
	
	return DC_OK;
}

/*
 * Function: texthandler_string
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the string question type
 * Assumptions: none
 */
static int texthandler_string(struct frontend *obj, struct question *q)
{
	char buf[1024] = {0};
	const char *defval = question_getvalue(q, "");
	if (defval)
		printf(_("[default = %s]"), defval);
	printf("> "); fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	CHOMP(buf);
#if defined(__s390__) || defined (__s390x__)
	if (buf[0] == '.' && buf[1] == 0 && defval == 0)
		question_setvalue(q, "");
	else if ((buf[0] == 0 || (buf[0] == '.' && buf[1] == 0)) && defval != 0)
#else
	if (buf[0] == 0 && defval != 0)
#endif
		question_setvalue(q, defval);
	else
		question_setvalue(q, buf);
	return DC_OK;
}

/*
 * Function: texthandler_text
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the text question type
 * Assumptions: none
 */
static int texthandler_text(struct frontend *obj, struct question *q)
{
	char *out = 0;
	char buf[1024];
	int sz = 1;

        out = malloc(sz);
	printf(_("Enter . on a line by itself when you are done\n"));
	while (fgets(buf, sizeof(buf), stdin))
	{
		if (strcmp(buf, ".\n") == 0) break;
		sz += strlen(buf);
		out = realloc(out, sz);
		memcpy(out + sz - strlen(buf) - 1, buf, strlen(buf));
	}
	out[sz-1] = 0;
	question_setvalue(q, out);
	free(out);
	return DC_OK;
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

/*
 * Function: text_intitialize
 * Input: struct frontend *obj - frontend UI object
 *        struct configuration *cfg - configuration parameters
 * Output: int - DC_OK, DC_NOTOK
 * Description: initializes the text UI
 * Assumptions: none
 *
 * TODO: SIGINT is ignored by the text UI, otherwise it interfers with
 * the way question/answers are handled. this is probably not optimal
 */
static int text_initialize(struct frontend *obj, struct configuration *conf)
{
	obj->interactive = 1;
	signal(SIGINT, SIG_IGN);
	return DC_OK;
}

/*
 * Function: text_go
 * Input: struct frontend *obj - frontend object
 * Output: int - DC_OK, DC_NOTOK
 * Description: asks all pending questions
 * Assumptions: none
 */
static int text_go(struct frontend *obj)
{
	struct question *q = obj->questions;
	int i;
	int ret;
	int display_title = 1;

        for (; q != 0; q = q->next) 
	{
		for (i = 0; i < DIM(question_handlers); i++)
			if (strcmp(q->template->type, question_handlers[i].type) == 0) 
			{

				if (display_title)
				{
					printf("%s\n\n", obj->title);
					display_title = 0;
				}
				texthandler_displaydesc(obj, q);
				ret = question_handlers[i].handler(obj, q);
				if (ret == DC_OK)
					obj->qdb->methods.set(obj->qdb, q);
				else
					return ret;
				break;
			}
	}

	return DC_OK;
}

static void text_progress_start(struct frontend *ui, int min, int max, const char *title)
{
    DELETE(ui->progress_title);
	ui->progress_title = STRDUP(title);
    ui->progress_min = min;
    ui->progress_max = max;
    ui->progress_cur = min;

    printf("%s\n", title);
}

static void text_progress_step(struct frontend *ui, int step, const char *info)
{
    int width = getwidth(), i;
    char out[256];

    snprintf(out, sizeof(out), "[%.1f%%] %s",
        (double)(ui->progress_cur - ui->progress_min) / 
        (double)(ui->progress_max - ui->progress_min) * 100.0, info);
    if (strlen(out) > width - 7)
        out[width - 7] = 0;
    else
    {
        for (i = strlen(out); i < width - 7; i++)
            out[i] = ' ';
        out[i] = 0;
    }

    ui->progress_cur += step;

    printf("%s\r", out);
    fflush(stdout);
}

static void text_progress_stop(struct frontend *ui)
{
    INFO(INFO_DEBUG, "%s\n", __FUNCTION__);
    text_progress_step(ui, 0, "");
    printf("\n");
}

struct frontend_module debconf_frontend_module =
{
	initialize: text_initialize,
	go: text_go,
    progress_start: text_progress_start,
    progress_step: text_progress_step,
    progress_stop: text_progress_stop,
};
