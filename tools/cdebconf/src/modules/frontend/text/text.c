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
 * $Id: text.c,v 1.9 2002/05/18 22:31:02 tfheen Rel $
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

#ifndef _
#define _(x) x
#endif

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
			if (ioctl(fd, TIOCGWINSZ, &ws) == 0)
				res = ws.ws_col;
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
	wrap_print(question_description(q));
	wrap_print(question_extended_description(q));
}

/*
 * Function: texthandler_boolean
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the boolean question type
 * Assumptions: none
 */
static int texthandler_boolean(struct frontend *obj, struct question *q)
{
	char buf[30];
	int ans = -1;
	int def = -1;
	const char *defval;

	defval = question_defaultval(q);
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
		printf("%s%s> ", _("Prompt: yes/no"), (defval == NULL ? "" :
			(def == 0 ? _(", default=no") : _(", default=yes"))));

		fgets(buf, sizeof(buf), stdin);
		if (strcmp(buf, "yes\n") == 0)
			ans = 1;
		else if (strcmp(buf, "no\n") == 0)
			ans = 0;
		else if (defval && strcmp(buf, "\n") == 0)
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
	char *defaults[100] = {0};
	char selected[100] = {0};
	char answer[1024];
	int i, j, count, dcount, choice;

	count = strchoicesplit(question_choices(q), choices, DIM(choices));
	dcount = strchoicesplit(question_defaultval(q), defaults, DIM(defaults));

	if (dcount > 0)
		for (i = 0; i < count; i++)
			for (j = 0; j < dcount; j++)
				if (strcmp(choices[i], defaults[j]) == 0)
					selected[i] = 1;

	while(1)
	{
		for (i = 0; i < count; i++)
		{
			printf("%3d. %s%s\n", i+1, choices[i], 
				(selected[i] ? _(" (selected)") : ""));
			
		}

		printf(_("Prompt: 1 - %d, q to end> "), count);
		fgets(answer, sizeof(answer), stdin);
		if (answer[0] == 'q') break;

		choice = atoi(answer);
		if (choice > 0 && choice <= count)
		{
			if (selected[choice-1] == 0) 
				selected[choice-1] = 1;
			else
				selected[choice-1] = 0;
		}
	}

	answer[0] = 0;
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

/*
 * Function: texthandler_note
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the note question type
 * Assumptions: none
 */
static int texthandler_note(struct frontend *obj, struct question *q)
{
	int c;
	printf("[Press enter to continue]\n");
	do { c = fgetc(stdin); } while (c != '\r' && c != '\n');
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
	int i, count, choice = 1, def = -1;
	const char *defval = question_defaultval(q);

	count = strchoicesplit(question_choices(q), choices, DIM(choices));
	strchoicesplit(question_choices_translated(q), choices_translated, DIM(choices_translated));
	if (count > 1)
	{
		if (defval != NULL)
		{
			for (i = 0; i < count; i++)
				if (strcmp(choices[i], defval))
					def = i + 1;
		}

		do
		{
			for (i = 0; i < count; i++)
				printf("%3d. %s%s\n", i+1, choices_translated[i],
					(def == i ? _(" (default)") : ""));

			printf(_("Prompt: 1 - %d> "), count);
			fgets(answer, sizeof(answer), stdin);
			if (answer[0] == '\n')
				choice = def;
			else
				choice = atoi(answer);
		} while (choice <= 0 || choice > count);
	}
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
	const char *defval = question_defaultval(q);
	if (defval)
		printf(_("[default = %s]"), defval);
	printf("> "); fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	CHOMP(buf);
	if (buf[0] == 0 && defval != 0)
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

	printf("%s\n\n", obj->title);

	for (; q != 0; q = q->next)
	{
		for (i = 0; i < DIM(question_handlers); i++)
			if (strcmp(q->template->type, question_handlers[i].type) == 0)
			{
				texthandler_displaydesc(obj, q);
				ret = question_handlers[i].handler(obj, q);
				if (ret == DC_OK)
					obj->db->question_set(obj->db, q);
				else
					return ret;
				break;
			}
	}

	return DC_OK;
}

struct frontend_module debconf_frontend_module =
{
	initialize: text_initialize,
	go: text_go,
};
