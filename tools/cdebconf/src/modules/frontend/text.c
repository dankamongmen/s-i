#include "common.h"
#include "template.h"
#include "question.h"
#include "frontend.h"
#include "database.h"
#include "strutl.h"

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

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

static void wrap_print(const char *str)
{
	/* Simple greedy line-wrapper */
	const int width = getwidth() - 1;
	int len = STRLEN(str);
	char line[width+1];
	const char *s, *e, *end, *lb;

	if (str == 0) return;

	s = e = str;
	end = str + len;
	
	while (len > 0)
	{
		/* try to fit the most characters */
		e = s + width;
		
		if (e >= end) 
		{
			e = end;
		}
		else
		{
			while (e > s && isalnum(*e)) e--;
			e++;
		}
		/* no word-break point found, so just break the line */
		if (e == s) e = s + width;

		/* if there's an explicit linebreak, honor it */
		lb = strchr(s, '\n');
		if (lb != NULL && lb < e) e = lb + 1;

		strncpy(line, s, e-s);
		line[e-s] = 0;
		printf("%s%s", line, (lb == NULL || lb >= e ? "\n" : ""));

		len -= (e-s);
		s = e;
		while (*s == ' ') s++;
	}
}

static void texthandler_displaydesc(struct frontend *obj, struct question *q) 
{
	wrap_print(question_description(q));
	wrap_print(question_extended_description(q));
}

static int texthandler_boolean(struct frontend *obj, struct question *q)
{
	char buf[30];
	int ans = -1;
	int def = -1;
	texthandler_displaydesc(obj, q);
	if (q->template->defaultval)
	{
		if (strcmp(q->template->defaultval, "true") == 0)
			def = 1;
		else if (strcmp(q->template->defaultval, "false") == 0)
			def = 0;
	}

	/* turn of SIGINT before asking for input, otherwise things
	 * get very messy
	 */
	do {
		printf("%s / %s> ", (def == 1 ? "[yes]" : "yes"),
			(def == 0 ? "[no]" : "no"));

		fgets(buf, sizeof(buf), stdin);
		if (strcmp(buf, "yes\n") == 0)
			ans = 1;
		else if (strcmp(buf, "no\n") == 0)
			ans = 0;
		else if (q->template->defaultval && strcmp(buf, "\n") == 0)
			ans = def;
	} while (ans < 0);

	question_setvalue(q, (ans ? "true" : "false"));
	return DC_OK;
}

static int texthandler_multiselect(struct frontend *obj, struct question *q)
{
	char *choices[100] = {0};
	char *defaults[100] = {0};
	char selected[100] = {0};
	char answer[1024];
	int i, j, count, dcount, choice;

	texthandler_displaydesc(obj, q);

	count = strchoicesplit(question_choices(q), choices, DIM(choices));
	dcount = strchoicesplit(q->template->defaultval, defaults, DIM(defaults));

	if (dcount > 0)
		for (i = 0; i < count; i++)
			for (j = 0; j < dcount; j++)
				if (strcmp(choices[i], defaults[j]) == 0)
					selected[i] = 1;

	while(1)
	{
		for (i = 0; i < count; i++)
		{
			printf("%s %3d) %s\n", (selected[i] ? "*" : " "), i+1,
			       choices[i]);
			
		}

		printf("1 - %d, q to end> ", count);
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
	question_setvalue(q, answer);
	
	return DC_OK;
}

static int texthandler_note(struct frontend *obj, struct question *q)
{
	int c;
	texthandler_displaydesc(obj, q);
	printf("[Press enter to continue]\n");
	do { c = fgetc(stdin); } while (c != '\r' && c != '\n');
	return DC_OK;
}

static int texthandler_password(struct frontend *obj, struct question *q)
{
	struct termios oldt, newt;
	char passwd[256] = {0};
	int i = 0, c;

	texthandler_displaydesc(obj, q);

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
	passwd[i] = 0;
	tcsetattr(0, TCSANOW, &oldt);
	question_setvalue(q, passwd);
	return DC_OK;
}

static int texthandler_select(struct frontend *obj, struct question *q)
{
	char *choices[100] = {0};
	char answer[10];
	int i, count, choice, def = -1;

	texthandler_displaydesc(obj, q);

	count = strchoicesplit(question_choices(q), choices, DIM(choices));
	if (q->template->defaultval != NULL)
	{
		for (i = 0; i < count; i++)
			if (strcmp(choices[i], q->template->defaultval) == 0)
				def = i + 1;
	}

	do
	{
		for (i = 0; i < count; i++)
			printf("%3d) %s\n", i+1, choices[i]);

		if (def > 0)
			printf("1 - %d [default = %d]> ", count, def);
		else
			printf("1 - %d> ", count);
		fgets(answer, sizeof(answer), stdin);
		if (answer[0] == '\n')
			choice = def;
		else
			choice = atoi(answer);
	} while (choice <= 0 || choice > count);

	question_setvalue(q, choices[choice - 1]);
	for (i = 0; i < count; i++) 
		free(choices[i]);
	
	return DC_OK;
}

static int texthandler_string(struct frontend *obj, struct question *q)
{
	char buf[1024] = {0};
	texthandler_displaydesc(obj, q);
	fgets(buf, sizeof(buf), stdin);
	CHOMP(buf);
	question_setvalue(q, buf);
	return DC_OK;
}

static int texthandler_text(struct frontend *obj, struct question *q)
{
	char *out = 0;
	char buf[1024];
	int sz = 1;
	texthandler_displaydesc(obj, q);
	while (fgets(buf, sizeof(buf), stdin))
	{
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

static int text_initialize(struct frontend *obj, struct configuration *conf)
{
	obj->interactive = 1;
	signal(SIGINT, SIG_IGN);
	return DC_OK;
}

static int text_go(struct frontend *obj)
{
	struct question *q = obj->questions;
	int i;
	int ret;

	printf("%s\n", obj->title);
	for (i = 0; i < strlen(obj->title); i++) {
		printf("=");
	}
	printf("\n\n");

	for (; q != 0; q = q->next)
	{
		for (i = 0; i < DIM(question_handlers); i++)
			if (strcmp(q->template->type, question_handlers[i].type) == 0)
			{
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
