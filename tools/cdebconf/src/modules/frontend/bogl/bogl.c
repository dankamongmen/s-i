#include "common.h"
#include "template.h"
#include "question.h"
#include "frontend.h"
#include "database.h"
#include "strutl.h"

#include <bogl/bogl.h>
#include <bogl/bowl.h>

/* Any private variables we may need. */
struct uidata {
};

typedef int (*handler_t)(struct frontend *ui, struct question *q);

struct question_handlers {
	const char *type;
	handler_t handler;
};

int bogl_handler_boolean(struct frontend *ui, struct question *q);
int bogl_handler_multiselect(struct frontend *ui, struct question *q);
int bogl_handler_select(struct frontend *ui, struct question *q);
int bogl_handler_note(struct frontend *ui, struct question *q);
int bogl_handler_string(struct frontend *ui, struct question *q);

struct question_handlers question_handlers[] = {
	{ "boolean", bogl_handler_boolean },
	{ "multiselect", bogl_handler_multiselect },
	{ "select", bogl_handler_select },
	{ "note", bogl_handler_note },
	{ "string", bogl_handler_string },
        { "eror", bogl_handler_note },
};

static handler_t handler(const char *type)
{
	int i;
	for(i = 0; i < DIM(question_handlers); i++)
		if(! strcasecmp(type, question_handlers[i].type))
			return question_handlers[i].handler;
	return NULL;
}

/* FIXME: Needs disabled button widget. */
static void drawnavbuttons(struct frontend *ui, struct question *q)
{
	if(ui->methods.can_go_back(ui, q))
		bowl_new_button(_("Previous"), DC_GOBACK);
	if(ui->methods.can_go_forward(ui, q))
		bowl_new_button(_("Next"), DC_OK);
}

static void drawdesctop(struct frontend *ui, struct question *q)
{
	bowl_title(ui->title);
	bowl_new_text(question_get_field(q, "", "description"));
}

static void drawdescbot(struct frontend *ui, struct question *q)
{
	bowl_new_text(question_get_field(q, "", "extended_description"));
}

/* boolean requires a new widget, the radio button :( */
/* Pretend with buttons - loses default info. */
int bogl_handler_boolean(struct frontend *ui, struct question *q)
{
	/* Should just make bowl_new_checkbox be properly const-qualified. */
	char *desc = strdup(question_get_field(q, "", "description"));
	int ret;
	
#if 0
	char ans = ' ';

	if(q->value && *(q->value))
		ans = (strcmp(q->value, "true") == 0) ? '*' : ' ';
#endif
	
	bowl_flush();
	drawdesctop(ui, q);

	/* Should be:  bowl_new_radio(); drawnavbuttons(ui, q); */
	if(ui->methods.can_go_back(ui, q))
		bowl_new_button(_("Previous"), 0);
	bowl_new_button(_("Yes"), 1);
	bowl_new_button(_("No"), 2);

	drawdescbot(ui, q);
	bowl_layout();
	ret = bowl_run ();

	if(ret != 0)
		question_setvalue(q, (ret == 1) ? "true" : "false");

	free(desc);
	return (ret == 0) ? DC_GOBACK : DC_OK;
}

int bogl_handler_note(struct frontend *ui, struct question *q)
{
	bowl_flush();
	drawdesctop(ui, q);
	drawnavbuttons(ui, q);
	drawdescbot(ui, q);
	bowl_layout();
	return bowl_run();
}

int bogl_handler_select(struct frontend *ui, struct question *q)
{
	return DC_OK;
}

int bogl_handler_multiselect(struct frontend *ui, struct question *q)
{
	char **choices, **choices_translated, **defaults, *selected;
	int i, j, count, dcount, ret;
	const char *p;

	count = 0;
	p = question_get_field(q, NULL, "choices");
	if (*p)
	{
		count++;
		for(; *p; p++)
			if(*p == ',')
				count++;
	}

	if (count <= 0) return DC_NOTOK;

	choices = malloc(sizeof(char *) * count);
	strchoicesplit(question_get_field(q, NULL, "choices"), choices, count);
	choices_translated = malloc(sizeof(char *) * count);
	strchoicesplit(question_get_field(q, "", "choices"), choices_translated, count);
	selected = malloc(sizeof(char) * count);
	memset(selected, ' ', count);

	dcount = 1;
	for(p = question_get_field(q, NULL, "value"); *p; p++)
		if(*p == ',')
		  	dcount++;
	defaults = malloc(sizeof(char *) * dcount);
	dcount = strchoicesplit(question_get_field(q, NULL, "value"), defaults, dcount);
	for(j = 0; j < dcount; j++)
	{
		for(i = 0; i < count; i++)
			if(strcmp(choices[i], defaults[j]) == 0)
			{
				selected[i] = '*';
				break;
			}
		free(defaults[j]);
	}
	free(defaults);

	bowl_flush();
	drawdesctop(ui, q);
	bowl_new_checkbox(choices, selected, count, (count > 15) ? 15 : count);
	drawnavbuttons(ui, q);
	drawdescbot(ui, q);
	
	bowl_layout();
	ret = bowl_run();

	if(ret == DC_OK)
	{
		/* Be safe - allow for commas and spaces. */
		char *answer = malloc(strlen(question_get_field(q, "", "choices")) + 1 + count);
		answer[0] = 0;
		for(i = 0; i < count; i++)
			if (selected[i] == '*')
			{
				if(answer[0] != 0)
					strcat(answer, ", ");
				strcat(answer, choices_translated[i]);
			}
		question_setvalue(q, answer);
	}

	for(i = 0; i < count; i++)
		free(choices[i]);
	free(choices);

	return ret;
}

int bogl_handler_string(struct frontend *ui, struct question *q)
{
	char *s;
	int ret;

	bowl_flush();
	drawdesctop(ui, q);
	bowl_new_input(&s, question_get_field(q, NULL, "value"));
	drawnavbuttons(ui, q);
	drawdescbot(ui, q);
	bowl_layout();
	ret = bowl_run();
	
	if(ret == DC_OK)
		question_setvalue(q, s);
	free(s);
	
	return ret;
}

static int bogl_initialize(struct frontend *ui, struct configuration *cfg)
{
	ui->interactive = 1;
	ui->data = NULL;

	bowl_init();
	
	bowl_done ();
	
	return DC_OK;
}

static int bogl_shutdown(struct frontend *ui)
{
	bowl_done();
	
	return DC_OK;
}

static int bogl_go(struct frontend *ui)
{
	struct question *q = ui->questions;
	int ret;
	
	bowl_init ();

	while(q)
	{
		ret = DC_OK;
		(*handler(q->template->type))(ui, q);
		if(ret == DC_OK)
			ui->qdb->methods.set(ui->qdb, q);
		else
			return ret;
		
		q = q->next;
	}
	
	bowl_done ();

	return DC_OK;
}

struct frontend_module debconf_frontend_module = {
	initialize: bogl_initialize,
	shutdown: bogl_shutdown,
	go: bogl_go,
};
