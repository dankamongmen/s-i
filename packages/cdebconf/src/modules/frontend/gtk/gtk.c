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
 * $Id$
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
#include "cdebconf_gtk.h"

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>

#include <gtk/gtk.h>

#define q_get_extended_description(q)   question_get_field((q), "", "extended_description")
#define q_get_description(q)  		question_get_field((q), "", "description")
#define q_get_choices(q)		question_get_field((q), "", "choices")
#define q_get_choices_vals(q)		question_get_field((q), NULL, "choices")
#define q_get_indices(q)		question_get_field((q), "", "indices")

/* A struct to let a question handler store appropriate set functions that will be called after
   gtk_main has quit */
struct setter_struct
{
    void (*func) (void*, struct question*);
    void *data;
    struct question *q;
    struct setter_struct *next;
};

typedef int (custom_func_t)(struct frontend*, struct question*, GtkWidget*);

void register_setter(void (*func)(void*, struct question*), 
		     void *data, struct question *q, struct frontend *obj)
{
    struct setter_struct *s;
    
    s = malloc(sizeof(struct setter_struct));
    s->func = func;
    s->data = data;
    s->q = q;
    s->next = ((struct frontend_data*)obj->data)->setters;
    ((struct frontend_data*)obj->data)->setters = s;
}

void free_description_data( GtkObject *obj, struct frontend_question_data* data )
{
    free(data);
}

gboolean 
show_description (GtkWidget *widget, struct frontend_question_data* data)
{
  struct question *q;
  struct frontend *obj;

  GtkWidget *main_window;
  GtkWidget *dialog;
  
  obj = data->obj;
  q = data->q;

  main_window = ((struct frontend_data*) obj->data)->window;

  dialog = gtk_message_dialog_new (GTK_WINDOW(main_window), GTK_DIALOG_MODAL,
				   GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
				   q_get_extended_description(q));

  gtk_dialog_run (GTK_DIALOG(dialog));
  gtk_widget_destroy (dialog);

    /* FIXME: no longer has a description frame
       GtkWidget *target;
       
       obj = data->obj;
       q = data->q;
       target = ((struct frontend_data*)obj->data)->description_label;
       
       gtk_label_set_text(GTK_LABEL(target), q_get_extended_description(q));
    */
    return FALSE;
}

GtkWidget*
create_help_button (struct frontend_question_data *data)
{
  GtkWidget *button;

  button = gtk_button_new_from_stock (GTK_STOCK_HELP);
  g_signal_connect (G_OBJECT(button), "clicked",
		    G_CALLBACK(show_description), data);

  return button;
}

gboolean is_first_question (struct question *q)
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

static void bool_setter(void *check, struct question *q)
{
    question_setvalue(q, (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)) ? "true" : "false"));
}	

static void entry_setter(void *entry, struct question *q)
{
    question_setvalue(q, gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void combo_setter(void *entry, struct question *q)
{
    char **choices, **choices_translated;
    int i, count;
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);
    
    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0)
        return /* DC_NOTOK */;
    choices = malloc(sizeof(char *) * count);
    choices_translated = malloc(sizeof(char *) * count);
    tindex = malloc(sizeof(int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
        return /* DC_NOTOK */;

    for (i = 0; i < count; i++)
    {
        if (strcmp(gtk_entry_get_text(GTK_ENTRY(entry)), choices_translated[i]) == 0)
	    question_setvalue(q, choices[tindex[i]]);

	free(choices[tindex[i]]);
	free(choices_translated[i]);
    }
    free(choices);
    free(choices_translated);
    free(tindex);
}	

static void multi_setter(void *check_container, struct question *q)
{
    gchar *result = NULL;
    gchar *copy = NULL;
    GList *check_list;
    int i, count;
    char **choices, **choices_translated;
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);

    check_list = gtk_container_get_children(GTK_CONTAINER(check_container));
    while(check_list)
    {
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_list->data)))
	{
            count = strgetargc(q_get_choices_vals(q));
            if (count <= 0)
                return /* DC_NOTOK */;
            choices = malloc(sizeof(char *) * count);
            choices_translated = malloc(sizeof(char *) * count);
            tindex = malloc(sizeof(int) * count);
            if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
                return /* DC_NOTOK */;

	    for (i = 0; i < count; i++)
            {
		if (strcmp(gtk_button_get_label(GTK_BUTTON(check_list->data)), choices_translated[i]) == 0)
		{
		    if(result != NULL)
		    {
			copy = g_strdup(result);
			free(result);
			result = g_strconcat(copy, ", ", choices[tindex[i]], NULL);
			free(copy);
		    }
		    else
			result = g_strdup(choices[tindex[i]]);
		}    
		free(choices[tindex[i]]);
		free(choices_translated[i]);
            }
            free(choices);
            free(choices_translated);
	    free(tindex);
	}
	check_list = g_list_next(check_list);
    }
    if(!result)
	result = g_strdup("");
    question_setvalue(q, result);
    g_list_free(check_list);
    free(result);
}

void call_setters(struct frontend *obj)
{
    struct setter_struct *s, *p;

    s = ((struct frontend_data*)obj->data)->setters;
    while (s != NULL)
    {
        (*s->func)(s->data, s->q);
        p = s;
        s = s->next;
        free(p);
    }
}

void button_single_callback(GtkWidget *button, struct frontend_question_data* data)
{
    struct frontend *obj = data->obj;
    struct question *q = data->q;
    char **choices, **choices_translated;
    int i, count;
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0)
        return /* DC_NOTOK */;
    choices = malloc(sizeof(char *) * count);
    choices_translated = malloc(sizeof(char *) * count);
    tindex = malloc(sizeof(int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
        return /* DC_NOTOK */;
    for (i = 0; i < count; i++)
    {
	if (strcmp(gtk_button_get_label(GTK_BUTTON(button)), choices_translated[i]) == 0)
	    question_setvalue(q, choices[tindex[i]]);

	free(choices[tindex[i]]);
	free(choices_translated[i]);

    }
    free(choices);
    free(choices_translated);
    free(tindex);
    ((struct frontend_data*)obj->data)->button_val = DC_OK;
    free(data);
   
    gtk_main_quit();
}

void check_toggled_callback (GtkWidget *toggle, gpointer data)
{
  struct question *q = (struct question*)data;
  gboolean value;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(toggle));
  bool_setter (toggle, q);
}

void boolean_single_callback(GtkWidget *button, struct frontend_question_data* data )
{
    struct frontend *obj = data->obj;
    struct question *q = data->q;
    char *ret;
    
    ret = (char*) gtk_object_get_user_data(GTK_OBJECT(button));
    question_setvalue(q, ret);
    free(ret);

    ((struct frontend_data*)obj->data)->button_val = DC_OK;;
    
    gtk_main_quit();
}

void exit_button_callback(GtkWidget *button, struct frontend* obj)
{
    int value;
    void *ret;
    
    ret = gtk_object_get_user_data(GTK_OBJECT(button));
    value = *(int*) ret;

    ((struct frontend_data*)obj->data)->button_val = value;
    
    gtk_main_quit();
}

static const char *
get_text(struct frontend *obj, const char *template, const char *fallback )
{
	        struct question *q = obj->qdb->methods.get(obj->qdb, template);
		        return q ? q_get_description(q) : fallback;
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

static int 
gtkhandler_boolean_single(struct frontend *obj, struct question *q, 
			  GtkWidget *qbox)
{
  GtkWidget *hbox;
  GtkWidget *check_button;
  GtkWidget *help_button;
  struct frontend_question_data *data;
  const char *defval = question_getvalue(q, "");
  
  data = NEW(struct frontend_question_data);
  data->obj = obj;
  data->q = q;
  
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX(qbox), hbox, TRUE, TRUE, 5);

  check_button = gtk_check_button_new_with_label (q_get_description (q));
  g_signal_connect (G_OBJECT(check_button), "toggled",
		    G_CALLBACK(check_toggled_callback),
		    q);
  gtk_box_pack_start (GTK_BOX(hbox), check_button, TRUE, TRUE, 5);
  
  help_button = create_help_button (data);
  gtk_box_pack_start (GTK_BOX(hbox), help_button, FALSE, FALSE, 3);

  /* FIXME: sensitive to the druid button 
     if (obj->methods.can_go_back(obj, q) == FALSE)
     {
     gtk_widget_set_sensitive(back_button, FALSE);
     }
  */
  
  if (strcmp (defval, "true") == 0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check_button), TRUE);
  
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
	
    check = gtk_check_button_new_with_label(q_get_description(q));
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
    char **choices, **choices_translated, **defvals;
    int i, j, count, defcount;
    struct frontend_question_data *data;
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0)
        return DC_NOTOK;
    choices = malloc(sizeof(char *) * count);
    choices_translated = malloc(sizeof(char *) * count);
    tindex = malloc(sizeof(int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
        return DC_NOTOK;

    defvals = malloc(sizeof(char *) * count);
    defcount = strchoicesplit(question_getvalue(q, ""), defvals, count);
    if (defcount <= 0) return DC_NOTOK;	

    check_container = gtk_vbox_new (FALSE, 0);

    g_signal_connect (G_OBJECT(check_container), "destroy", G_CALLBACK (free_description_data), data);

    for (i = 0; i < count; i++) 
    {
	check = gtk_check_button_new_with_label(choices_translated[i]);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);
	for (j = 0; j < defcount; j++)
        {
	    if (strcmp(choices[tindex[i]], defvals[j]) == 0)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
        }
        /* TODO: This should go somewhere that doesn't interfere. "Help"
         * that prevents you from actually doing anything is not helpful.
         */
        /* g_signal_connect (G_OBJECT(check), "enter", G_CALLBACK (show_description), data); */
        gtk_box_pack_start(GTK_BOX(check_container), check, FALSE, FALSE, 0);
	if (is_first_question(q) && (i == 0) )
	    gtk_widget_grab_focus(check);

	free(choices[tindex[i]]);
        free(choices_translated[i]);
    }
    free(choices);
    free(choices_translated);
    free(tindex);
    for (j = 0; j < defcount; j++)
        free(defvals[j]);
    free(defvals);

    frame = gtk_frame_new(q_get_description(q));
    gtk_container_add(GTK_CONTAINER (frame), check_container);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    register_setter(multi_setter, check_container, q, obj);

    return DC_OK;
}

static int gtkhandler_note(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *label;
	
    label = gtk_label_new (q_get_extended_description(q));
    gtk_misc_set_alignment(GTK_MISC (label), 0.0, 0.0);
    gtk_label_set_line_wrap(GTK_LABEL (label), TRUE);

    frame = gtk_frame_new(q_get_description(q));
    gtk_container_add(GTK_CONTAINER (frame), label);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);
	
    return DC_OK;
}

static int gtkhandler_text(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    /* FIXME: text probably shouldn't be quite so interactive as note */
    return gtkhandler_note(obj, q, qbox);
}

static int gtkhandler_password(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *entry;
    struct frontend_question_data *data;
	
    entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
    frame = gtk_frame_new(q_get_description(q));
    gtk_container_add(GTK_CONTAINER (frame), entry);	

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    if (is_first_question(q))
	gtk_widget_grab_focus(entry);

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    g_signal_connect (G_OBJECT(entry), "destroy", G_CALLBACK (free_description_data), data);
    /* TODO: showing description on keyboard focus grab makes it impossible
     * to enter a password.
     */
    /* g_signal_connect (G_OBJECT(entry), "grab-focus", G_CALLBACK (show_description), data); */
	
    register_setter(entry_setter, entry, q, obj);

    return DC_OK;
}

static int gtkhandler_select_single(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *button, *button_box;
    char **choices, **choices_translated;
    int i, count;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0)
        return DC_NOTOK;
    choices = malloc(sizeof(char *) * count);
    choices_translated = malloc(sizeof(char *) * count);
    tindex = malloc(sizeof(int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
        return DC_NOTOK;

    button_box = gtk_vbutton_box_new();

    frame = gtk_frame_new(q_get_description(q));
    gtk_container_add(GTK_CONTAINER (frame), button_box);

    gtk_box_pack_start(GTK_BOX(qbox), frame, FALSE, FALSE, 5);

    for (i = 0; i < count; i++)
    {
        button = gtk_button_new_with_label(choices_translated[i]);
	gtk_object_set_user_data(GTK_OBJECT(button), choices[tindex[i]]);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (button_single_callback), data);	
	gtk_box_pack_start(GTK_BOX(button_box), button, FALSE, FALSE, 5);
	if (defval && strcmp(choices[tindex[i]], defval) == 0)
	{
	    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	    gtk_widget_grab_focus(button);
	    gtk_widget_grab_default(button);
	}
	free(choices[tindex[i]]);
    }
    free(choices);
    free(choices_translated);
    free(tindex);

    return DC_OK;
}

static int gtkhandler_select_multiple(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *combo, *frame;
    GList *items = NULL;
    char **choices, **choices_translated;
    struct frontend_question_data *data;
    int i, count;
    const char *defval = question_getvalue(q, "");
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);
    
    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0)
        return DC_NOTOK;
    choices = malloc(sizeof(char *) * count);
    choices_translated = malloc(sizeof(char *) * count);
    tindex = malloc(sizeof(int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
        return DC_NOTOK;
    free(choices);
    free(tindex);
    if (count <= 0) return DC_NOTOK;

    for (i = 0; i < count; i++)
        /* steal memory */
        items = g_list_append (items, choices_translated[i]);
    free(choices_translated);

    combo = gtk_combo_new ();
    gtk_combo_set_popdown_strings (GTK_COMBO (combo), items);
    g_list_free(items);
    gtk_editable_set_editable (GTK_EDITABLE(GTK_COMBO(combo)->entry), FALSE);
    gtk_entry_set_text (GTK_ENTRY(GTK_COMBO(combo)->entry), defval);
    gtk_combo_set_value_in_list (GTK_COMBO (combo), TRUE, FALSE);

    frame = gtk_frame_new(q_get_description(q));
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
    if (defval)
	gtk_entry_set_text (GTK_ENTRY(entry), defval);
    gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
    frame = gtk_frame_new(q_get_description(q));
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

/*
static int gtkhandler_custom(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    custom_func_t *custom_func;
    char *symname, *tmp;
    int ret, i;
    
    symname = strdup(q->template->tag);
    for (i = 0; i < strlen(symname); i++) 
    {
	if (symname[i] == '/')
	    symname[i] = '_';
    }

    asprintf(&tmp, "%s_gtk", symname);

    custom_func = (custom_func_t*) dlsym(q->custom_module, tmp);
    free(tmp);
    free(symname);

    ret = custom_func(obj, q, qbox);

    return ret;
}
*/

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
    { "error",	        gtkhandler_note },
//  { "custom",         gtkhandler_custom },
    { "text",           gtkhandler_text }
};

void set_window_properties(GtkWidget *window)
{
    gtk_widget_set_size_request (window, 600, 400);
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
}

void set_design_elements(struct frontend *obj, GtkWidget *window)
{
    GtkWidget *mainbox;
    GtkWidget *targetbox, *targetbox_scroll;
    GtkWidget *actionbox;
    GtkWidget *button_next, *button_prev;
    int *ret_val;

    mainbox = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER(mainbox), 5);
    targetbox = gtk_vbox_new (FALSE, 10);
    ((struct frontend_data*) obj->data)->target_box = targetbox;

    targetbox_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (targetbox_scroll), targetbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (targetbox_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    actionbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout (GTK_BUTTON_BOX(actionbox), GTK_BUTTONBOX_END); 
    gtk_box_pack_end (GTK_BOX(mainbox), actionbox, FALSE, FALSE, 5);

    button_prev = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
    ret_val = NEW(int);
    *ret_val = DC_GOBACK;
    gtk_object_set_user_data (GTK_OBJECT(button_prev), ret_val);
    g_signal_connect (G_OBJECT(button_prev), "clicked",
                      G_CALLBACK(exit_button_callback), obj);
    gtk_box_pack_start (GTK_BOX(actionbox), button_prev, TRUE, TRUE, 2);

    button_next = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
    ret_val = NEW(int);
    *ret_val = DC_OK;
    gtk_object_set_user_data (GTK_OBJECT(button_next), ret_val);
    /* the question is held by a gtk_main thing */
    g_signal_connect (G_OBJECT(button_next), "clicked",
                      G_CALLBACK(exit_button_callback), obj);
    gtk_box_pack_start (GTK_BOX(actionbox), button_next, TRUE, TRUE, 2);
    GTK_WIDGET_SET_FLAGS (button_next, GTK_CAN_DEFAULT);

    ((struct frontend_data*) obj->data)->button_prev = button_prev;
    ((struct frontend_data*) obj->data)->button_next = button_next;

    gtk_box_pack_start(GTK_BOX (mainbox), targetbox_scroll, TRUE, TRUE, 5);
    gtk_container_add(GTK_CONTAINER(window), mainbox);
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

    return DC_OK;
}

static int gtk_go(struct frontend *obj)
{
    struct frontend_data *data = (struct frontend_data *) obj->data;
    struct question *q = obj->questions;
    int i;
    int ret;
    GtkWidget *questionbox;

    if (q == NULL) return DC_OK;

    data->setters = NULL;
    questionbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(data->target_box),
                       questionbox, FALSE, FALSE, 5);

    /* FIXME: no more description frame
       if (strcmp(q->template->type, "note") != 0 )
       gtk_label_set_text(GTK_LABEL(data->description_label), 
       q_get_extended_description(q)); 
    */
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
    q = obj->questions;
    gtk_window_set_title(GTK_WINDOW(data->window), obj->title);
    gtk_widget_set_sensitive (data->button_prev, obj->methods.can_go_back(obj, q));
    gtk_widget_grab_default(data->button_next);
    gtk_widget_show_all(data->window);
    gtk_main();
    if (data->button_val == DC_OK) 
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

    /* FIXME
      gtk_label_set_text(GTK_LABEL(data->description_label), ""); 
    */

    return data->button_val;
}

static bool gtk_can_go_back(struct frontend *obj, struct question *q)
{
    return (obj->capability & DCF_CAPB_BACKUP);
}

static void gtk_progress_start(struct frontend *obj, int min, int max, const char *title)
{
    GtkWidget *progress_bar, *target_box, *frame;

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
    while (gtk_events_pending ())
	gtk_main_iteration ();
}

static void gtk_progress_set(struct frontend *obj, int val)
{
    gdouble progress;

    obj->progress_cur = val;
    if (obj->progress_max - obj->progress_min > 0)
    {
        progress = (gdouble)(obj->progress_cur - obj->progress_min) /
                   (gdouble)(obj->progress_max - obj->progress_min);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(((struct frontend_data*)obj->data)->progress_bar),
				      progress);
    }
    gtk_widget_show_all(((struct frontend_data*)obj->data)->window);
    while (gtk_events_pending ())
	gtk_main_iteration ();
}

static void gtk_progress_info(struct frontend *obj, const char *info)
{
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(((struct frontend_data*)obj->data)->progress_bar), info);
    gtk_widget_show_all(((struct frontend_data*)obj->data)->window);
    while (gtk_events_pending ())
	gtk_main_iteration ();
}

static void gtk_progress_stop(struct frontend *obj)
{
    gtk_widget_destroy(gtk_widget_get_parent(((struct frontend_data*)obj->data)->progress_bar));
    gtk_widget_show_all(((struct frontend_data*)obj->data)->window);
    while (gtk_events_pending ())
	gtk_main_iteration ();
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
