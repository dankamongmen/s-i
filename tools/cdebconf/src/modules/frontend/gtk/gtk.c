/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: gtk.c
 *
 * Description: gtk UI for cdebconf
 * Some notes on the implementation - optimistic at best. 
 *  mbc - just to get this off of the ground, Im' creating a dialog
 *        and calling gtk_main for each question. once I get the tests
 *        running, I'll probably send a delete_event signal in the
 *        next and back button callbacks. 
 *    
 *        There is some rudimentary attempt at implementing the next
 *        and back functionality. 
 *
 * $Id: gtk.c,v 1.11 2003/03/23 23:35:12 sley Exp $
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

#include <gtk/gtk.h>

/* Use this struct to store data that shall live between the questions */
struct frontend_data
{
    GtkWidget *window; //Main window of the frontend
    GtkWidget *description_label; //Pointer to the Description Field
    GtkWidget *target_box; //Pointer to the box, where question widgets shall be stored in
    struct setter_struct *setters; //Struct to register the Set Functions of the Widgets
};

/* Embed frontend ans question in this object to pass it through an event handler */
struct show_description_data
{
    struct frontend *obj;
    struct question *q;
};

/* A struct to let a question handler store appropriate set functions that will be called after
   gtk_main has quit */
struct setter_struct
{
    void (*func) (GtkWidget *, struct question *);
    GtkWidget *widget;
    struct question *q;
    struct setter_struct *next;
};

static void bool_setter(GtkWidget *check, struct question *q)
{
    question_setvalue(q, (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)) ? "true" : "false"));
}	

static void entry_setter(GtkWidget *entry, struct question *q)
{
    question_setvalue(q, gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void combo_setter(GtkWidget *entry, struct question *q)
{
    gchar *choices[100] = {0};
    gchar *choices_translated[100] = {0};
    int i, count;
    
    count = strchoicesplit(question_get_field(q, NULL, "choices"),
			   choices, DIM(choices));
    strchoicesplit(question_get_field(q, "", "choices"),
		   choices_translated, DIM(choices_translated));
    for (i = 0; i < count; i++)
    {
        if (strcmp(gtk_entry_get_text(GTK_ENTRY(entry)), choices_translated[i]) == 0)
        {
	    question_setvalue(q, choices[i]);
            free(choices[i]);
            free(choices_translated[i]);
	}
    }
}	

static void multi_setter(GtkWidget *check_container, struct question *q)
{
    gchar *result = NULL;
    gchar *copy = NULL;
    GList *check_list;
    int i, count;
    gchar *choices[100] = {0};
    gchar *choices_translated[100] = {0};

    check_list = gtk_container_get_children(GTK_CONTAINER(check_container));
    while(check_list)
    {
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_list->data)))
	{
	    count = strchoicesplit(question_get_field(q, NULL, "choices"),
				   choices, DIM(choices));
	    strchoicesplit(question_get_field(q, "", "choices"),
			   choices_translated, DIM(choices_translated));
		
	    for (i = 0; i < count; i++)
            {
		if (strcmp(gtk_button_get_label(GTK_BUTTON(check_list->data)), choices_translated[i]) == 0)
		{
		    if(result != NULL)
		    {
			copy = g_strdup(result);
			free(result);
			result = g_strconcat(copy, ", ", choices[i], NULL);
			free(copy);
                        free(choices[i]);
                        free(choices_translated[i]);
		    }
		    else
                    {
			result = g_strdup(choices[i]);
                        free(choices[i]);
                        free(choices_translated[i]);
                    }
		}    
            }
	}
	check_list = g_list_next(check_list);
    }
    question_setvalue(q, result);
    g_list_free(check_list);
    free(result);
}

void register_setter(void (*func)(GtkWidget *, struct question *), 
		     GtkWidget *w, struct question *q, struct frontend *obj)
{
    struct setter_struct *s;
    
    s = malloc(sizeof(struct setter_struct));
    s->func = func;
    s->widget = w;
    s->q = q;
    s->next = ((struct frontend_data*)obj->data)->setters;
    ((struct frontend_data*)obj->data)->setters = s;
}

void call_setters(struct frontend *obj)
{
    struct setter_struct *s, *p;

    s = ((struct frontend_data*)obj->data)->setters;
    while (s != NULL)
    {
        (*s->func)(s->widget, s->q);
        p = s;
        s = s->next;
        free(p);
    }
}

void free_description_data( GtkObject *obj, struct show_description_data* data )
{
    free(data);
}

static gboolean show_description( GtkWidget *widget, struct show_description_data* data )
{
    struct question *q;
    struct frontend *obj;
    GtkWidget *target;

    obj = data->obj;
    q = data->q;
    target = ((struct frontend_data*)obj->data)->description_label;

    gtk_label_set_text(GTK_LABEL(target), question_get_field(q, "", "extended_description"));

    return FALSE;
}

void add_buttons(struct frontend *obj, GtkWidget *qbox)
{
    GtkWidget *nextButton, *separator, *buttonbox;

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (qbox), separator, FALSE, 0, 0);

    nextButton = gtk_button_new_with_label("Finish");

    g_signal_connect (G_OBJECT (nextButton), "clicked",
                      G_CALLBACK (gtk_main_quit), NULL);

    buttonbox = gtk_hbutton_box_new();
    gtk_box_pack_start (GTK_BOX(buttonbox), nextButton, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(qbox), buttonbox, FALSE, FALSE, 5);
}

static int gtkhandler_boolean(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *check;
    struct show_description_data *data;
    const char *defval = question_getvalue(q, "");

    data = NEW(struct show_description_data);
    data->obj = obj;
    data->q = q;
	
    check = gtk_check_button_new_with_label(question_get_field(q, "", "description"));
    if (strcmp(defval, "true") == 0)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);
    g_signal_connect (G_OBJECT(check), "enter", G_CALLBACK (show_description), data);
    g_signal_connect (G_OBJECT(check), "grab-focus", G_CALLBACK (show_description), data);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER (frame), check);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);
	
    register_setter(bool_setter, check, q, obj);

    return DC_OK;
}

static int gtkhandler_multiselect(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *check_container, *check;
    char *choices[100] ={0};
    char *choices_translated[100] = {0};
    char *defvals[100] = {0};
    int i, j, count, dcount;
    struct show_description_data *data;

    data = NEW(struct show_description_data);
    data->obj = obj;
    data->q = q;

    count = strchoicesplit(question_get_field(q, NULL, "choices"),
                           choices, DIM(choices));

    strchoicesplit(question_get_field(q, "", "choices"),
                   choices_translated, DIM(choices_translated));

    dcount = strchoicesplit(question_get_field(q, NULL, "value"),
                            defvals, DIM(defvals));
    if (count <= 0) return DC_NOTOK;	

    check_container = gtk_vbox_new (FALSE, 0);

    g_signal_connect (G_OBJECT(check_container), "destroy",
                      G_CALLBACK (free_description_data), data);

    for (i = 0; i < count; i++) 
    {
	check = gtk_check_button_new_with_label(choices_translated[i]);
	free(choices_translated[i]);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);
	for (j = 0; j < dcount; j++)
        {
	    if (strcmp(choices[i], defvals[j]) == 0)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
        }
        g_signal_connect (G_OBJECT(check), "enter", G_CALLBACK (show_description), data);
        gtk_box_pack_start(GTK_BOX(check_container), check, FALSE, FALSE, 0);
    }

    frame = gtk_frame_new(question_get_field(q, "", "description"));
    gtk_container_add(GTK_CONTAINER (frame), check_container);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);
	
    register_setter(multi_setter, check_container, q, obj);

    return DC_OK;
}

static int gtkhandler_note(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *label;
	
    label = gtk_label_new (question_get_field(q, "", "extended_description"));
    gtk_misc_set_alignment(GTK_MISC (label), 0.0, 0.0);
    gtk_label_set_line_wrap(GTK_LABEL (label), TRUE);

    frame = gtk_frame_new(question_get_field(q, "", "description"));
    gtk_container_add(GTK_CONTAINER (frame), label);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);
	
    return DC_OK;
}

static int gtkhandler_password(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *entry;
    struct show_description_data *data;
	
    entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
    frame = gtk_frame_new(question_get_field(q, "", "description"));
    gtk_container_add(GTK_CONTAINER (frame), entry);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    data = NEW(struct show_description_data);
    data->obj = obj;
    data->q = q;

    g_signal_connect (G_OBJECT(entry), "destroy",
                      G_CALLBACK (free_description_data), data);

    g_signal_connect (G_OBJECT(entry), "grab-focus", G_CALLBACK (show_description), data);
	
    register_setter(entry_setter, entry, q, obj);

    return DC_OK;
}

static int gtkhandler_select(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *combo, *frame;
    GList *items = NULL;
    gchar *choices_translated[100] = {0};
    struct show_description_data *data;
    int i, count;
    const char *defval = question_getvalue(q, "");

    count = strchoicesplit(question_get_field(q, "", "choices"),
                           choices_translated, DIM(choices_translated));
	
    if (count <= 0) return DC_NOTOK;

    for (i = 0; i < count; i++)
        items = g_list_append (items, choices_translated[i]);

    combo = gtk_combo_new ();
    gtk_combo_set_popdown_strings (GTK_COMBO (combo), items);
    g_list_free(items);
    gtk_editable_set_editable (GTK_EDITABLE(GTK_COMBO(combo)->entry), FALSE);
    gtk_entry_set_text (GTK_ENTRY(GTK_COMBO(combo)->entry), defval);
    gtk_combo_set_value_in_list (GTK_COMBO (combo), TRUE, FALSE);

    frame = gtk_frame_new(question_get_field(q, "", "description"));
    gtk_container_add(GTK_CONTAINER (frame), combo);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    data = NEW(struct show_description_data);
    data->obj = obj;
    data->q = q;

    g_signal_connect (G_OBJECT(GTK_COMBO(combo)->entry), "destroy",
                      G_CALLBACK (free_description_data), data);

    g_signal_connect (G_OBJECT(GTK_COMBO(combo)->entry), "grab-focus",
                      G_CALLBACK (show_description), data);

    register_setter(combo_setter, GTK_COMBO(combo)->entry, q, obj);

    return DC_OK;
}

static int gtkhandler_string(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *entry;
    struct show_description_data *data;
    const char *defval = question_getvalue(q, "");
	
    entry = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY(entry), defval);
    gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    frame = gtk_frame_new(question_get_field(q, "", "description"));
    gtk_container_add(GTK_CONTAINER (frame), entry);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    data = NEW(struct show_description_data);
    data->obj = obj;
    data->q = q;

    g_signal_connect (G_OBJECT(entry), "destroy",
                      G_CALLBACK (free_description_data), data);

    g_signal_connect (G_OBJECT(entry), "grab-focus", G_CALLBACK (show_description), data);

    register_setter(entry_setter, entry, q, obj);

    return DC_OK;
}

/* ----------------------------------------------------------------------- */
struct question_handlers {
    const char *type;
    int (*handler)(struct frontend *obj, struct question *q, GtkWidget *questionbox);
} question_handlers[] = {
    { "boolean",        gtkhandler_boolean },
    { "multiselect",    gtkhandler_multiselect },
    { "note",	        gtkhandler_note },
    { "password",	gtkhandler_password },
    { "select",	        gtkhandler_select },
    { "string",	        gtkhandler_string },
    //	{ "text",	gtkhandler_text }
};

void test_window_full()
{
}

void set_window_properties(GtkWidget *window)
{
    gtk_widget_set_size_request (window, 600, 400);
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
}

void set_design_elements(struct frontend *obj, GtkWidget *window)
{
    GtkWidget *mainbox, *targetbox, *description_area, *description_frame,
        *description_scroll, *targetbox_scroll;

    description_area = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC (description_area), 0.0, 0.0);
    gtk_label_set_line_wrap(GTK_LABEL (description_area), TRUE);

    ((struct frontend_data*) obj->data)->description_label = description_area;
    description_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (description_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (description_scroll), description_area);
    gtk_widget_set_size_request (description_scroll, 600, 100);

    description_frame = gtk_frame_new("Description");
    gtk_container_add(GTK_CONTAINER (description_frame), description_scroll);
	
    mainbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(mainbox), 5);
    gtk_box_pack_end(GTK_BOX (mainbox), description_frame, FALSE, FALSE, 5);
    targetbox = gtk_vbox_new(FALSE, 10);
    ((struct frontend_data*) obj->data)->target_box = targetbox;

    targetbox_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (targetbox_scroll), targetbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (targetbox_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_box_pack_start(GTK_BOX (mainbox), targetbox_scroll, TRUE, TRUE, 5);
    gtk_container_add(GTK_CONTAINER(window), mainbox);
}

static int gtk_initialize(struct frontend *obj, struct configuration *conf)
{
    GtkWidget *window;
    int args = 1;
    char **name;

    (char**) name = malloc(2 * sizeof(char*));
    (char*) name[0] = malloc(9 * sizeof(char));
    name[0] = "cdebconf";
    name[1] = NULL;

    obj->data = NEW(struct frontend_data);
    obj->interactive = 1;
	
    gtk_init (&args, &name);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    set_window_properties (window);
    set_design_elements (obj, window);
    ((struct frontend_data*) obj->data)->window = window;

    return DC_OK;
}

static int gtk_go(struct frontend *obj)
{
    struct question *q = obj->questions;
    int i;
    int ret;
    GtkWidget *questionbox;

    if (q == NULL) return DC_OK;

    ((struct frontend_data*) obj->data)->setters = NULL;
    questionbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX (((struct frontend_data*)obj->data)->target_box),
                       questionbox, FALSE, FALSE, 5);

    if (strcmp(q->template->type, "note") != 0 )
        gtk_label_set_text(GTK_LABEL( ((struct frontend_data*)obj->data)->description_label), 
                           question_get_field(q, "", "extended_description")); 
    while (q != 0)
    {
        for (i = 0; i < DIM(question_handlers); i++)
            if (strcmp(q->template->type, question_handlers[i].type) == 0)
            {
                test_window_full();
                ret = question_handlers[i].handler(obj, q, questionbox);
                if (ret != DC_OK)
                {
                    return ret;
                }
            }
        q = q->next;
    }

    add_buttons(obj, questionbox);
    gtk_widget_show_all(((struct frontend_data*)obj->data)->window);
    gtk_main();
    call_setters(obj);
    gtk_widget_destroy(questionbox);
    gtk_label_set_text(GTK_LABEL( ((struct frontend_data*)obj->data)->description_label),""); 
    q = obj->questions;
    while (q != NULL)
    {
        obj->qdb->methods.set(obj->qdb, q);
        q = q->next;
    }
	
    return DC_OK;
}

struct frontend_module debconf_frontend_module =
{
    initialize: gtk_initialize,
    go: gtk_go,
};
