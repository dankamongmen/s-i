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
 * $Id: gtk.c,v 1.20 2003/05/11 11:59:15 sjogren Exp $
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
#include <pthread.h>

#include <gtk/gtk.h>

/* Use this struct to store data that shall live between the questions */
struct frontend_data
{
    GtkWidget *window; //Main window of the frontend
    GtkWidget *description_label; //Pointer to the Description Field
    GtkWidget *target_box; //Pointer to the box, where question widgets shall be stored in
    GtkWidget *progress_bar; //Pointer to the Progress Bar when initialized
    struct setter_struct *setters; //Struct to register the Set Functions of the Widgets
    int button_val; //Value of the button pressed to leave a form
    pthread_t *gtkthread; //Thread that runs gtk_main and handlers
    pthread_mutex_t *gtkmain_lock; //Lock when gtk_main must not be runned
    pthread_cond_t *data_ready;// Main thread will wait until gtkthread has evaluated the widgets
};

/* Embed frontend ans question in this object to pass it through an event handler */
struct frontend_question_data
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
    
    //FIXME: use user-data property of GtkObject to transport the choicename
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
	    //FIXME: use user-data property of GtkObject to transport the choicename
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

void free_description_data( GtkObject *obj, struct frontend_question_data* data )
{
    free(data);
}

void button_single_callback(GtkWidget *button, struct frontend_question_data* data)
{
    struct frontend *obj = data->obj;
    struct question *q = data->q;

    question_setvalue(q, (gchar*) gtk_object_get_user_data(GTK_OBJECT(button)) );
    ((struct frontend_data*)obj->data)->button_val = DC_OK;

    free(data);
   
    pthread_cond_signal(((struct frontend_data*)obj->data)->data_ready);
}

void exit_button_callback(GtkWidget *button, struct frontend* obj)
{
    ((struct frontend_data*)obj->data)->button_val =
	*((int*)gtk_object_get_user_data(GTK_OBJECT(button))) ;

    pthread_cond_signal(((struct frontend_data*)obj->data)->data_ready);
}

static gboolean show_description( GtkWidget *widget, struct frontend_question_data* data )
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

gboolean need_continue_button(struct frontend *obj)
{
    if (obj->questions->next == NULL)
    {
	if (strcmp(obj->questions->template->type, "boolean") == 0)
	    return FALSE;
	else if (strcmp(obj->questions->template->type, "select") == 0)
	    return FALSE;
    }
    return TRUE;
}

gboolean need_back_button(struct frontend *obj)
{
    if (obj->questions->next == NULL)
    {
	if (strcmp(obj->questions->template->type, "boolean") == 0)
	    return FALSE;
    }
    return TRUE;
}

gboolean is_first_question(struct question *q)
{
    struct question *crawl;

    crawl = q;

    while (crawl->prev != NULL)
    {
	if (strcmp(crawl->prev->template->type, "note") != 0)
	    return FALSE;
	crawl = crawl->prev;
    }
    return TRUE;
}

void add_buttons(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *separator, *button_box;
    GtkWidget *continue_button = NULL;
    GtkWidget *back_button = NULL;
    int *ret_val;

    if (need_continue_button(obj))
    {
	continue_button = gtk_button_new_with_label("Continue");
	ret_val = NEW(int);
	*ret_val = DC_OK;
	gtk_object_set_user_data(GTK_OBJECT(continue_button), ret_val);
	g_signal_connect (G_OBJECT (continue_button), "clicked", G_CALLBACK (exit_button_callback), obj);
    }

    if (need_back_button(obj))
    {
	back_button = gtk_button_new_with_label("Go Back");
	ret_val = NEW(int);
	*ret_val = DC_GOBACK;
	gtk_object_set_user_data(GTK_OBJECT(back_button), ret_val);
	g_signal_connect (G_OBJECT (back_button), "clicked", G_CALLBACK (exit_button_callback), obj);

	if (obj->methods.can_go_back(obj, q) == FALSE)
	{
	    gtk_widget_set_sensitive(back_button, FALSE);
	}
    }

    if (continue_button || back_button)
    {
	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (qbox), separator, FALSE, 0, 0);

	button_box = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(qbox), button_box, FALSE, FALSE, 5);

	gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_SPREAD);

	if (back_button)
	    gtk_box_pack_start (GTK_BOX(button_box), back_button, FALSE, FALSE, 5);

	if (continue_button) 
	{
	    gtk_box_pack_start (GTK_BOX(button_box), continue_button, FALSE, FALSE, 5);
	    GTK_WIDGET_SET_FLAGS (continue_button, GTK_CAN_DEFAULT);
	    gtk_widget_grab_default(continue_button);
	}
    }
}

static int gtkhandler_boolean_single(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *yes_button, *no_button, *back_button, *button_box, *vbox, *description_label;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");
    int *ret_val;

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;
	
    yes_button = gtk_button_new_with_label("Yes");
    no_button = gtk_button_new_with_label("No");
    back_button = gtk_button_new_with_label("Go Back");

    ret_val = NEW(int);
    *ret_val = DC_GOBACK;

    gtk_object_set_user_data(GTK_OBJECT(back_button), ret_val);
    gtk_object_set_user_data(GTK_OBJECT(yes_button), g_strdup("true"));
    gtk_object_set_user_data(GTK_OBJECT(no_button), g_strdup("false"));

    g_signal_connect (G_OBJECT (back_button), "clicked", G_CALLBACK (exit_button_callback), obj);
    g_signal_connect (G_OBJECT (yes_button), "clicked", G_CALLBACK (button_single_callback), data);
    g_signal_connect (G_OBJECT (no_button), "clicked", G_CALLBACK (button_single_callback), data);

    if (obj->methods.can_go_back(obj, q) == FALSE)
    {
	gtk_widget_set_sensitive(back_button, FALSE);
    }

    button_box = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_SPREAD);
    gtk_box_pack_start(GTK_BOX(button_box), back_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), yes_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), no_button, FALSE, FALSE, 5);

    description_label = gtk_label_new(question_get_field(q, "", "extended_description"));
    gtk_misc_set_alignment(GTK_MISC (description_label), 0.0, 0.0);
    gtk_label_set_line_wrap(GTK_LABEL (description_label), TRUE);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), description_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 5);

    frame = gtk_frame_new(question_get_field(q, "", "description"));
    gtk_container_add(GTK_CONTAINER (frame), vbox);

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    if (strcmp (defval, "true") == 0)
    {
	GTK_WIDGET_SET_FLAGS (yes_button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(yes_button);
	gtk_widget_grab_focus(yes_button);
    }
    else
    {
	GTK_WIDGET_SET_FLAGS (no_button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(no_button);
	gtk_widget_grab_focus(no_button);
    }

    gtk_label_set_text(GTK_LABEL(((struct frontend_data*) obj->data)->description_label), "");
	
    return DC_OK;
}

static int gtkhandler_boolean_multiple(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *check;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;
	
    check = gtk_check_button_new_with_label(question_get_field(q, "", "description"));
    if (strcmp(defval, "true") == 0)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);
    g_signal_connect (G_OBJECT(check), "enter", G_CALLBACK (show_description), data);
    g_signal_connect (G_OBJECT(check), "grab-focus", G_CALLBACK (show_description), data);
    g_signal_connect (G_OBJECT(check), "destroy", G_CALLBACK (free_description_data), data);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER (frame), check);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    if (is_first_question(q))
	gtk_widget_grab_focus(check);
	
    register_setter(bool_setter, check, q, obj);

    return DC_OK;
}

static int gtkhandler_boolean(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    if (q->next == NULL && q->prev == NULL)
        return gtkhandler_boolean_single(obj, q, qbox);
    else
        return gtkhandler_boolean_multiple(obj, q, qbox);
}

static int gtkhandler_multiselect(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *check_container, *check;
    char *choices[100] ={0};
    char *choices_translated[100] = {0};
    char *defvals[100] = {0};
    int i, j, count, dcount;
    struct frontend_question_data *data;

    data = NEW(struct frontend_question_data);
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

    g_signal_connect (G_OBJECT(check_container), "destroy", G_CALLBACK (free_description_data), data);

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
	if (is_first_question(q) && (i == 0) )
	    gtk_widget_grab_focus(check);
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
    struct frontend_question_data *data;
	
    entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
    frame = gtk_frame_new(question_get_field(q, "", "description"));
    gtk_container_add(GTK_CONTAINER (frame), entry);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    if (is_first_question(q))
	gtk_widget_grab_focus(entry);

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    g_signal_connect (G_OBJECT(entry), "destroy", G_CALLBACK (free_description_data), data);
    g_signal_connect (G_OBJECT(entry), "grab-focus", G_CALLBACK (show_description), data);
	
    register_setter(entry_setter, entry, q, obj);

    return DC_OK;
}

static int gtkhandler_select_single(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *button, *button_box;
    gchar *choices_translated[100] = {0};
    gchar *choices[100] = {0};
    int i, count;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    count = strchoicesplit(question_get_field(q, NULL, "choices"),
                           choices, DIM(choices));
	
    strchoicesplit(question_get_field(q, "", "choices"),
		   choices_translated, DIM(choices_translated));

    if (count <= 0) return DC_NOTOK;

    button_box = gtk_vbutton_box_new();

    frame = gtk_frame_new(question_get_field(q, "", "description"));
    gtk_container_add(GTK_CONTAINER (frame), button_box);

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    for (i = 0; i < count; i++)
    {
        button = gtk_button_new_with_label(choices_translated[i]);
	gtk_object_set_user_data(GTK_OBJECT(button), choices[i]);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (button_single_callback), data);	
	gtk_box_pack_start(GTK_BOX(button_box), button, FALSE, FALSE, 5);
	if (strcmp(choices[i], defval) == 0)
	{
	    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	    gtk_widget_grab_focus(button);
	    gtk_widget_grab_default(button);
	}

	free(choices_translated);
    }

    return DC_OK;
}

static int gtkhandler_select_multiple(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *combo, *frame;
    GList *items = NULL;
    gchar *choices_translated[100] = {0};
    struct frontend_question_data *data;
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

    if (is_first_question(q))
	gtk_widget_grab_focus(combo);

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    g_signal_connect (G_OBJECT(GTK_COMBO(combo)->entry), "destroy",
                      G_CALLBACK (free_description_data), data);

    g_signal_connect (G_OBJECT(GTK_COMBO(combo)->entry), "grab-focus",
                      G_CALLBACK (show_description), data);

    register_setter(combo_setter, GTK_COMBO(combo)->entry, q, obj);

    return DC_OK;
}

static int gtkhandler_select(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    if (q->next == NULL && q->prev == NULL)
        return gtkhandler_select_single(obj, q, qbox);
    else
        return gtkhandler_select_multiple(obj, q, qbox);
}

static int gtkhandler_string(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *entry;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");
	
    entry = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY(entry), defval);
    gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    frame = gtk_frame_new(question_get_field(q, "", "description"));
    gtk_container_add(GTK_CONTAINER (frame), entry);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    if (is_first_question(q))
	gtk_widget_grab_focus(entry);

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    g_signal_connect (G_OBJECT(entry), "destroy", G_CALLBACK (free_description_data), data);
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

void set_window_properties(GtkWidget *window)
{
    gtk_widget_set_size_request (window, 600, 400);
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
}

GtkWidget* setup_log()
{
    GtkWidget *logarea;

    logarea = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(logarea), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(logarea), GTK_WRAP_CHAR);
    return logarea;
}


void set_design_elements(struct frontend *obj, GtkWidget *window)
{
    GtkWidget *mainbox, *targetbox, *description_area, *description_frame,
        *description_scroll, *targetbox_scroll, *notebook, *label_main, *label_log;

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

    label_main = gtk_label_new("Debian Installer");
    label_log = gtk_label_new("Log Output");

    notebook = gtk_notebook_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), mainbox, label_main);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), setup_log(), label_log); 

    gtk_container_add(GTK_CONTAINER(window), notebook);
}

static void init_gtkmain(struct frontend* obj)
{
    while (TRUE) 
    {
	if(gtk_events_pending ())
	{
	    pthread_mutex_lock(((struct frontend_data*) obj->data)->gtkmain_lock);
	    while (gtk_events_pending ())
	    {
		gtk_main_iteration ();
	    }
	    pthread_mutex_unlock(((struct frontend_data*) obj->data)->gtkmain_lock);
	}
    }
}

static int gtk_initialize(struct frontend *obj, struct configuration *conf)
{
    GtkWidget *window;
    int args = 1;
    char **name;

    //FIXME: This can surely be done in a better way
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
    gtk_widget_show_all(window);

    ((struct frontend_data*) obj->data)->gtkmain_lock = NEW(pthread_mutex_t);
    pthread_mutex_init(((struct frontend_data*) obj->data)->gtkmain_lock, NULL);

    ((struct frontend_data*) obj->data)->data_ready = NEW(pthread_cond_t);
    pthread_cond_init(((struct frontend_data*) obj->data)->data_ready, NULL);

    ((struct frontend_data*) obj->data)->gtkthread = NEW(pthread_t);
    pthread_create(((struct frontend_data*) obj->data)->gtkthread, NULL,
		   (void*) &init_gtkmain, (void*) obj);

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
    pthread_mutex_lock(((struct frontend_data*) obj->data)->gtkmain_lock);
    if (strcmp(q->template->type, "note") != 0 )
        gtk_label_set_text(GTK_LABEL( ((struct frontend_data*)obj->data)->description_label), 
                           question_get_field(q, "", "extended_description")); 
    while (q != 0)
    {
        for (i = 0; i < DIM(question_handlers); i++)
            if (strcmp(q->template->type, question_handlers[i].type) == 0)
            {
                ret = question_handlers[i].handler(obj, q, questionbox);
                if (ret != DC_OK)
                {
                    return ret;
                }
            }
        q = q->next;
    }

    add_buttons(obj, q, questionbox);
    gtk_widget_show_all(((struct frontend_data*)obj->data)->window);
    pthread_cond_wait(((struct frontend_data*) obj->data)->data_ready,
		      ((struct frontend_data*) obj->data)->gtkmain_lock);
    if ( ((struct frontend_data*)obj->data)->button_val == DC_OK ) 
    {
	call_setters(obj);
	q = obj->questions;
	while (q != NULL)
	{
	    obj->qdb->methods.set(obj->qdb, q);
	    q = q->next;
	}
    }
    gtk_widget_destroy(questionbox);
    gtk_label_set_text(GTK_LABEL( ((struct frontend_data*)obj->data)->description_label),""); 
    ret = ((struct frontend_data*)obj->data)->button_val;
    pthread_mutex_unlock(((struct frontend_data*) obj->data)->gtkmain_lock);

    return ret;
}

static int gtk_can_go_back(struct frontend *obj, struct question *q)
{
    return (obj->capability & DCF_CAPB_BACKUP);
}

static void gtk_progress_start(struct frontend *obj, int min, int max, const char *title)
{
    GtkWidget *progress_bar, *target_box, *frame;

    pthread_mutex_lock(((struct frontend_data*) obj->data)->gtkmain_lock);

    obj->progress_title = NULL;
    obj->progress_min = min;
    obj->progress_max = max;
    obj->progress_cur = min;

    target_box = ((struct frontend_data*)obj->data)->target_box;
    progress_bar = gtk_progress_bar_new ();
    ((struct frontend_data*)obj->data)->progress_bar = progress_bar;
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "");

    frame = gtk_frame_new(title);
    gtk_container_add(GTK_CONTAINER (frame), progress_bar);

    gtk_box_pack_start(GTK_BOX(target_box), frame, FALSE, FALSE, 5);
    gtk_widget_show_all(((struct frontend_data*)obj->data)->window);
    pthread_mutex_unlock(((struct frontend_data*) obj->data)->gtkmain_lock);
}

static void gtk_progress_set(struct frontend *obj, int val)
{
    gdouble progress;

    pthread_mutex_lock(((struct frontend_data*) obj->data)->gtkmain_lock);
    obj->progress_cur = val;
    if (obj->progress_max - obj->progress_min > 0)
    {

        progress = (gdouble)(obj->progress_cur - obj->progress_min) /
                   (gdouble)(obj->progress_max - obj->progress_min);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(((struct frontend_data*)obj->data)->progress_bar),
				      progress);
    }
    gtk_widget_show_all(((struct frontend_data*)obj->data)->window);
    pthread_mutex_unlock(((struct frontend_data*) obj->data)->gtkmain_lock);
}

static void gtk_progress_info(struct frontend *obj, const char *info)
{
    pthread_mutex_lock(((struct frontend_data*) obj->data)->gtkmain_lock);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(((struct frontend_data*)obj->data)->progress_bar), info);
    gtk_widget_show_all(((struct frontend_data*)obj->data)->window);
    pthread_mutex_unlock(((struct frontend_data*) obj->data)->gtkmain_lock);
}

static void gtk_progress_stop(struct frontend *obj)
{
    pthread_mutex_lock(((struct frontend_data*) obj->data)->gtkmain_lock);
    gtk_widget_destroy(gtk_widget_get_parent(((struct frontend_data*)obj->data)->progress_bar));
    gtk_widget_show_all(((struct frontend_data*)obj->data)->window);
    pthread_mutex_unlock(((struct frontend_data*) obj->data)->gtkmain_lock);
}


struct frontend_module debconf_frontend_module =
{
    initialize: gtk_initialize,
    go: gtk_go,
    can_go_back: gtk_can_go_back,
    progress_start: gtk_progress_start,
    progress_info: gtk_progress_info,
    progress_set: gtk_progress_set,
    progress_stop: gtk_progress_stop,
};
