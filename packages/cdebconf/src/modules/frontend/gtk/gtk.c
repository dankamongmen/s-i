/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: gtk.c
 *
 * Description: gtk UI for cdebconf
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
#include "plugin.h"
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
#include <syslog.h>

#include <debian-installer/slist.h>

#include <gtk/gtk.h>


/* used to set internal horizontal padding between a question and questionbox frame */
#define QUESTIONBOX_HPADDING 6
/* used to set vertical padding between two adjacent questions inside questionbox */
#define QUESTIONBOX_VPADDING 3


/* used by the treeview widgets */
enum
{
	COL_NAME = 0,
	NUM_COLS
} ;

typedef int (gtk_handler)(struct frontend *obj, struct question *q, GtkWidget *questionbox);

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

/* If the user decides to "jump" we should always give him a first chance to
 * go back, to jump saving the data he entered or to jump discarding changes;
 * actually the first option is still unimplemented because i don't know how
 * to properly SETUP the signal handling needed to close the dialog window
 * without quitting the gtk_main()
*/
gboolean jump_confirmation (GtkWidget *widget, struct frontend_question_data* data)
{
    struct question *q;
    struct frontend *obj;

    GtkWidget *main_window;
    GtkWidget *dialog, *label;
    gint result;
  
    obj = data->obj;
    q = data->q;

    main_window = ((struct frontend_data*) obj->data)->window;

    dialog = gtk_dialog_new_with_buttons ("My dialog",
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_ACCEPT,
                                          GTK_STOCK_NO,
                                          GTK_RESPONSE_NO,
                                          /* GTK_STOCK_CANCEL, */
                                          /* GTK_RESPONSE_REJECT, */
                                          NULL);

	/* TODO: The following string should be more explicative and should be localized too */
    label = gtk_label_new ("Do you want to save your changes before jumping?");
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
    gtk_widget_show_all (dialog);
    result = gtk_dialog_run ( GTK_DIALOG(dialog) );
    gtk_widget_destroy (dialog);

    return result;
}


gboolean show_description (GtkWidget *widget, struct frontend_question_data* data)
{
    struct question *q;
    struct frontend *frontend_ptr;
    struct frontend_data *frontend_data_ptr;
    GtkWidget *view;  		
    GtkTextBuffer *buffer;
  
    frontend_ptr = data->obj;
    frontend_data_ptr = frontend_ptr->data;
    view = (GtkWidget*)frontend_data_ptr->info_box;
    q = data->q;
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view) );

	/* Workaround for questions missing extended description */
    if (strlen(q_get_extended_description(q)) > 0)
    	gtk_text_buffer_set_text (buffer, q_get_extended_description(q), -1);
    else if (strlen(q_get_description(q)) > 0)
    	gtk_text_buffer_set_text (buffer, q_get_description(q), -1);

    /* INFO(INFO_DEBUG, "GTK_DI - show_description(%s) called", q_get_extended_description(q)); */

    return DC_OK;
}


/* used to clear the help area */
gboolean clear_description (GtkWidget *widget, struct frontend_question_data* data)
{
    struct frontend *frontend_ptr;
    struct frontend_data *frontend_data_ptr;
    GtkWidget *view;

    GtkTextBuffer *buffer;

    frontend_ptr=data->obj;
    frontend_data_ptr=frontend_ptr->data;
    view=(GtkWidget*)frontend_data_ptr->info_box;

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view) );

    gtk_text_buffer_set_text (buffer, "", -1);

    return DC_OK;
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

    /* INFO(INFO_DEBUG, "GTK_DI - call_setters() called"); */

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

    INFO(INFO_DEBUG, "GTK_DI - check_toggled_callback() called");
    value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(toggle));
    bool_setter (toggle, q);
}

void boolean_single_callback(GtkWidget *button, struct frontend_question_data* data )
{
    struct frontend *obj = data->obj;
    struct question *q = data->q;
    char *ret;

    INFO(INFO_DEBUG, "GTK_DI - boolean_single_callback() called");
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

    INFO(INFO_DEBUG, "GTK_DI - exit_button_callback() called, value: %d", value);

    ((struct frontend_data*)obj->data)->button_val = value;
    
    gtk_main_quit();
}


/* this callback function is called by gtkhandler_select_single_jump() only
 * and schedules the jump by copying in fe_data->jump_target the tag
 * of the question the user wants to jump to
 */
void jump_callback(GtkWidget *button, struct frontend_question_data* data)
{
    struct frontend *obj = data->obj;
    struct frontend_data *fe_data = obj->data;
    struct question *q = data->q;
    char **choices, **choices_translated;
    int i, count;
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);
    gint ret_val = GTK_RESPONSE_NONE;

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0)
        return /* DC_NOTOK */;
    choices = malloc(sizeof(char *) * count);
    choices_translated = malloc(sizeof(char *) * count);
    tindex = malloc(sizeof(int) * count);

    INFO(INFO_DEBUG, "GTK_DI - button_single_callback(%s) called",
         gtk_button_get_label(GTK_BUTTON(button)));

    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
        return /* DC_NOTOK */;
    for (i = 0; i < count; i++)
    {
        if (strcmp(gtk_button_get_label(GTK_BUTTON(button)), choices_translated[i]) == 0) 
        {	
            if( fe_data->ask_jump_confirmation == FALSE )
        	{
        		/* If the user jumps without having touched any widget there is no
        		 * need to ask him if he wants to save the changes he made.
        		 * This includes the case of a single note or text question, where
        		 * the user can do no modifications.
        		 */
        		ret_val=GTK_RESPONSE_NO;
        	}
        	else if( strcmp(data->obj->questions->tag,"languagechooser/language-name")==0 )
        	{
        		/* This is just a temporary workaround to prevent the debian-installer from
        		 * crashing when performing inter-language jumps.
        		 */
        		ret_val=GTK_RESPONSE_NO;
				INFO(INFO_DEBUG, "GTK_DI - jump workaround to the inter language jump bug occourred" );
        	}
        	else
        		{
	            ret_val=jump_confirmation ( button, data);
	            /* INFO(INFO_DEBUG, "GTK_DI - jump confirmation return value: %d", ret_val); */
	            }
            switch (ret_val)
            {
                case GTK_RESPONSE_ACCEPT:
                    /* the user wants to save the changes made up to that
                     * point and then do the jump
                     */ 
                    strcpy(fe_data->jump_target,choices[tindex[i]]);
                    ((struct frontend_data*)obj->data)->button_val = DC_OK;
                    INFO(INFO_DEBUG, "GTK_DI - jump programmed, modifications confirmed, target: \"%s\"", fe_data->jump_target);
                    break;

                case GTK_RESPONSE_NO:
                    /* the user wants to forget the changes made up to that
                     * point and then do the jump
                     */ 
                    strcpy(fe_data->jump_target,choices[tindex[i]]);
                    ((struct frontend_data*)obj->data)->button_val = DC_GOBACK;
                    INFO(INFO_DEBUG, "GTK_DI - jump programmed, modifications canceled, target: \"%s\"", fe_data->jump_target);
                    break;

                default:
                    /* The user changed his mind and wants to cancel the jump. 
                     * This is also the case GTK_RESPONSE_REJECT but until
                     * the "cancel the jump" option works program control will
                     * never reach this point
                     */
                    INFO(INFO_DEBUG, "GTK_DI - jump canceled");
                    break;
            }
        }
        free(choices[tindex[i]]);
        free(choices_translated[i]);
    }

    free(choices);
    free(choices_translated);
    free(tindex);
    free(data);
   
    if( (ret_val==GTK_RESPONSE_ACCEPT) | (ret_val==GTK_RESPONSE_NO) )
        gtk_main_quit();
}

/* signals the need to ask the user if he wants to save changes before jumping */
void enable_jump_confirmation_callback(GtkWidget *widget, struct frontend_question_data* data)
{
	struct frontend *obj;
	struct frontend_data *fe_data;
	obj=data->obj;
	fe_data=obj->data;
	fe_data->ask_jump_confirmation = TRUE;
	
}

gboolean select_treeview_callback (GtkTreeSelection *selection, GtkTreeModel  *model, GtkTreePath *path, gboolean path_currently_selected, struct frontend_question_data *data)
{
    struct question *q = data->q;
    char **choices, **choices_translated;
    int i, count;
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);
	gchar *name;
	GtkTreeIter iter;
	
	INFO(INFO_DEBUG, "GTK_DI - gboolean select_treeview_callback() called");

	enable_jump_confirmation_callback(NULL, data);

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0)
        return FALSE; /* DC_NOTOK */;
    choices = malloc(sizeof(char *) * count);
    choices_translated = malloc(sizeof(char *) * count);
    tindex = malloc(sizeof(int) * count);

    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
        return FALSE;/* DC_NOTOK */;

	if (gtk_tree_model_get_iter(model, &iter, path))
		{
	    gtk_tree_model_get(model, &iter, COL_NAME, &name, -1);
	    if (!path_currently_selected)
		    {
		    for (i = 0; i < count; i++)
				{
	    		if (strcmp(name, choices_translated[i]) == 0)
	    			{
	    			/* INFO(INFO_DEBUG, "GTK_DI - gboolean select_treeview_callback(): %s is going to be selected  called", name); */
	        		question_setvalue(q, choices[tindex[i]]);
	        		}
	    		free(choices[tindex[i]]);
	    		free(choices_translated[i]);
				}
		    }
		}
	
    g_free(name);
    free(choices);
    free(choices_translated);
    free(tindex);

return TRUE;
}

static const char *
get_text(struct frontend *obj, const char *template, const char *fallback )
{
    struct question *q = obj->qdb->methods.get(obj->qdb, template);
    return q ? q_get_description(q) : fallback;
}

static GtkTextDirection get_text_direction(struct frontend *obj)
{
	const char *dirstr = get_text(obj, "debconf/text-direction", "LTR - default text direction");
	if (dirstr[0] == 'R')
		return GTK_TEXT_DIR_RTL;
	return GTK_TEXT_DIR_LTR;
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
    GtkWidget *frame, *hpadbox;
    GtkWidget *check_button;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");

    INFO(INFO_DEBUG, "GTK_DI - gtkhandler_boolean_single() called");

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    check_button = gtk_check_button_new_with_label (q_get_description (q));
	if (strcmp (defval, "true") == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check_button), TRUE);
	g_signal_connect (G_OBJECT(check_button), "toggled", G_CALLBACK(enable_jump_confirmation_callback), data);
    g_signal_connect (G_OBJECT(check_button), "toggled", G_CALLBACK(check_toggled_callback), q);
    g_signal_connect (G_OBJECT(check_button), "enter", G_CALLBACK (show_description), data);

    frame = gtk_frame_new(q_get_description (q));
    gtk_container_add(GTK_CONTAINER (frame), check_button);
	hpadbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX(hpadbox), frame, TRUE, TRUE, QUESTIONBOX_HPADDING);
	gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE, TRUE, QUESTIONBOX_VPADDING);

    return DC_OK;
}

static int gtkhandler_boolean_multiple(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *check, *hpadbox;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");

    INFO(INFO_DEBUG, "GTK_DI - gtkhandler_boolean_multiple() called");

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
    g_signal_connect (G_OBJECT(check), "toggled", G_CALLBACK (enable_jump_confirmation_callback), data);
    g_signal_connect (G_OBJECT(check), "destroy", G_CALLBACK (free_description_data), data);

    frame = gtk_frame_new(q_get_description (q));
    gtk_container_add(GTK_CONTAINER (frame), check);	

	hpadbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX(hpadbox), frame, TRUE, TRUE, QUESTIONBOX_HPADDING);
	gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE, TRUE, QUESTIONBOX_VPADDING);

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
    GtkWidget *frame, *check_container, *check, *hpadbox;
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
    if (defcount < 0)
        return DC_NOTOK;
    /* This is to prevent multiselect questions with no options from
     * making the frontend hang.
     * TODO: the frontend should also automatically
     * skip the question and return DC_OK.
     * The following two lines of code need to be commented in order to allow
     * multiselect questions with options but no default options activated
     * from being ignored by the frontend.
     *
     * else if (defcount == 0)
     * return DC_OK; 
     */

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
        g_signal_connect (G_OBJECT(check), "toggled", G_CALLBACK (enable_jump_confirmation_callback), data);
        g_signal_connect (G_OBJECT(check), "enter", G_CALLBACK (show_description), data);
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
	hpadbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX(hpadbox), frame, TRUE, TRUE, QUESTIONBOX_HPADDING);
	gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE, TRUE, QUESTIONBOX_VPADDING);

    register_setter(multi_setter, check_container, q, obj);

    return DC_OK;
}

static int gtkhandler_note(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *label, *hpadbox;

    INFO(INFO_DEBUG, "GTK_DI - gtkhandler_note() called");

    label = gtk_label_new (q_get_extended_description(q));
    gtk_misc_set_alignment(GTK_MISC (label), 0.0, 0.0);
    gtk_label_set_line_wrap(GTK_LABEL (label), TRUE);

    frame = gtk_frame_new(q_get_description(q));
    gtk_container_add(GTK_CONTAINER (frame), label);	
	hpadbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX(hpadbox), frame, TRUE, TRUE, QUESTIONBOX_HPADDING);
	gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE, TRUE, QUESTIONBOX_VPADDING);
	
    return DC_OK;
}

static int gtkhandler_text(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    /* FIXME: text probably shouldn't be quite so interactive as note */
    return gtkhandler_note(obj, q, qbox);
}

static int gtkhandler_password(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *entry, *hpadbox;
    struct frontend_question_data *data;
    INFO(INFO_DEBUG, "GTK_DI - gtkhandler_password() called");

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

    if (is_first_question(q))
		gtk_widget_grab_focus(entry);

    g_signal_connect (G_OBJECT(entry), "destroy", G_CALLBACK (free_description_data), data);
    g_signal_connect (G_OBJECT(entry), "backspace", G_CALLBACK (enable_jump_confirmation_callback), data);
    /*
     * TODO: also when inserting or deleting text the need to ask user if he wants to
     * save changes before jumping should be communicated with an appropriate callback.
     * at the moment the two following lines do not work properly.
     * g_signal_connect (G_OBJECT(entry), "delete-from-cursor", G_CALLBACK (enable_jump_confirmation_callback), data);
     * g_signal_connect (G_OBJECT(entry), "insert-at-cursor", G_CALLBACK (enable_jump_confirmation_callback), data);
     */
    g_signal_connect (G_OBJECT(entry), "grab-focus", G_CALLBACK (show_description), data);

	frame = gtk_frame_new(q_get_description(q));
	gtk_container_add(GTK_CONTAINER (frame), entry );
	hpadbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX(hpadbox), frame, TRUE, TRUE, QUESTIONBOX_HPADDING);
	gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE, TRUE, QUESTIONBOX_VPADDING);
	
    register_setter(entry_setter, entry, q, obj);

    return DC_OK;
}

/* This functions diplays the main menu, if enabled, in the left part of the screen.
 * Input parameters
 * bool sensitive_buttons : makes the buttons sensitive/unsensitive
 * bool enable_jumping : indicates the appropriate callback function 
 */
static int gtkhandler_main_menu(struct frontend *obj, struct question *q, GtkWidget *qbox, bool sensitive_buttons, bool jump_enabled)
{
    GtkWidget *label, *button, *button_box;
    char **choices, **choices_translated;
    int i, count;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);

	INFO(INFO_DEBUG, "GTK_DI - gtkhandler_main_menu() called");

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

    label = gtk_label_new(q_get_description(q));
    gtk_box_pack_start(GTK_BOX(qbox), label, FALSE, FALSE, 5);

    button_box = gtk_vbutton_box_new();
    gtk_vbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);
    gtk_box_pack_start(GTK_BOX(qbox), button_box, FALSE, FALSE, 5);

    for (i = 0; i < count; i++)
    {
        button = gtk_button_new_with_label(choices_translated[i]);
        gtk_object_set_user_data(GTK_OBJECT(button), choices[tindex[i]]);


        if (sensitive_buttons == FALSE)
	        gtk_widget_set_sensitive( GTK_WIDGET(button), FALSE );

        if (jump_enabled == TRUE)
	        g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK (jump_callback), data);
	    else
   	        g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK (button_single_callback), data);

        /* g_signal_connect (G_OBJECT(button), "enter", G_CALLBACK (show_description), data); */
        /* g_signal_connect (G_OBJECT(button), "leave", G_CALLBACK (clear_description), data); */

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


static int gtkhandler_select_treeview(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    char **choices, **choices_translated;
    int i, count;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);

	GtkTreeModel        *model;
	GtkListStore  		*store;
	GtkTreeIter    		iter;
	GtkWidget           *view;
	GtkCellRenderer     *renderer;
	GtkTreeSelection    *selection;

    INFO(INFO_DEBUG, "GTK_DI - gtkhandler_select_treeview() called");

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

	view = gtk_tree_view_new ();
    gtk_box_pack_start(GTK_BOX(qbox), view, FALSE, FALSE, 0);
    
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), -1, q_get_description(q), renderer, "text", COL_NAME, NULL);
	store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_UINT);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc) select_treeview_callback, data, NULL);
	model = GTK_TREE_MODEL( store );
	gtk_tree_view_set_model (GTK_TREE_VIEW (view), model);
	g_object_unref (model);

    for (i = 0; i < count; i++)
    {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, COL_NAME, choices_translated[i], -1);
        if (defval && strcmp(choices[tindex[i]], defval) == 0)
        {
        	gtk_tree_selection_select_iter  (selection, &iter );
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
    GtkWidget *combo, *frame, *hpadbox;
    GList *items = NULL;
    char **choices, **choices_translated;
    struct frontend_question_data *data;
    int i, count;
    const char *defval = question_getvalue(q, "");
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_select_multiple() called"); */

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
    free(choices);
    free(tindex);
    if (count <= 0) return DC_NOTOK;

    for (i = 0; i < count; i++)
    {
        /* steal memory */
        items = g_list_append (items, choices_translated[i]);
        /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_select_multiple(\"%s\")", choices_translated[i]); */
    }
    free(choices_translated);

    combo = gtk_combo_new ();
    gtk_combo_set_popdown_strings (GTK_COMBO (combo), items);
    g_list_free(items);
    gtk_editable_set_editable (GTK_EDITABLE(GTK_COMBO(combo)->entry), FALSE);
    
    if (defval != NULL)
    	gtk_entry_set_text (GTK_ENTRY(GTK_COMBO(combo)->entry), defval);
    else
    	gtk_entry_set_text (GTK_ENTRY(GTK_COMBO(combo)->entry), "");
    gtk_combo_set_value_in_list (GTK_COMBO (combo), TRUE, FALSE);

    if (is_first_question(q))
        gtk_widget_grab_focus(combo);

    g_signal_connect (G_OBJECT(GTK_COMBO(combo)->entry), "destroy",
                      G_CALLBACK (free_description_data), data);
	g_signal_connect (G_OBJECT(GTK_COMBO(combo)->entry), "grab-focus",
                      G_CALLBACK (enable_jump_confirmation_callback), data);
    g_signal_connect (G_OBJECT(GTK_COMBO(combo)->entry), "grab-focus",
                      G_CALLBACK (show_description), data);

	frame = gtk_frame_new(q_get_description(q));
	gtk_container_add(GTK_CONTAINER (frame), combo);
	hpadbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX(hpadbox), frame, TRUE, TRUE, QUESTIONBOX_HPADDING);
	gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE, TRUE, QUESTIONBOX_VPADDING);

    register_setter(combo_setter, GTK_COMBO(combo)->entry, q, obj);

    return DC_OK;
}

static int gtkhandler_select(struct frontend *obj, struct question *q, GtkWidget *qbox)
{

    if (q->prev == NULL && q->next == NULL)
        return gtkhandler_select_treeview(obj, q, qbox);
    else
		return gtkhandler_select_multiple(obj, q, qbox);
}

static int gtkhandler_string(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *frame, *entry, *hpadbox;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");

    INFO(INFO_DEBUG, "GTK_DI - gtkhandler_string() called");

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    entry = gtk_entry_new ();
    if (defval != NULL)
        gtk_entry_set_text (GTK_ENTRY(entry), defval);
    else
        gtk_entry_set_text (GTK_ENTRY(entry), "");
    gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

    if (is_first_question(q))
        gtk_widget_grab_focus(entry);

    g_signal_connect (G_OBJECT(entry), "destroy", G_CALLBACK (free_description_data), data);
    /*
     * TODO: also when inserting or deleting text the need to ask user if he wants to
     * save changes before jumping should be communicated with an appropriate callback.
     * at the moment the two following lines do not work properly.
     * g_signal_connect (G_OBJECT(entry), "backspace", G_CALLBACK (enable_jump_confirmation_callback), data);
     * g_signal_connect (G_OBJECT(entry), "delete-from-cursor", G_CALLBACK (enable_jump_confirmation_callback), data);
     * g_signal_connect (G_OBJECT(entry), "insert-at-cursor", G_CALLBACK (enable_jump_confirmation_callback), data);
     */
    g_signal_connect (G_OBJECT(entry), "grab-focus", G_CALLBACK (show_description), data);

	frame = gtk_frame_new(q_get_description(q));
	gtk_container_add(GTK_CONTAINER (frame),entry );
	hpadbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX(hpadbox), frame, TRUE, TRUE, QUESTIONBOX_HPADDING);
	gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE, TRUE, QUESTIONBOX_VPADDING);

    register_setter(entry_setter, entry, q, obj);

    return DC_OK;
}



struct question_handlers {
    const char *type;
    gtk_handler *handler;
} question_handlers[] = {
    { "boolean",        gtkhandler_boolean },
    { "multiselect",    gtkhandler_multiselect },
    { "note",	        gtkhandler_note },
    { "password",	gtkhandler_password },
    { "select",	        gtkhandler_select },
    { "string",	        gtkhandler_string },
    { "error",	        gtkhandler_note },
    { "text",           gtkhandler_text },
    { "",               NULL },
};

void set_window_properties(GtkWidget *window)
{
    gtk_widget_set_size_request (window, 800, 600);
    gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
}

void set_design_elements(struct frontend *obj, GtkWidget *window)
{
    /* a scheme of the hierarchy of boxes used to store widgets

	0) Globalbox
	1) Mainbox
	2) menubox_vpad
	3) menubox_scroll
	9) menubox
	4) targetbox_scroll
	5) targetbox
	6) actionbox
	7) infobox
	8) progress_bar
	
	0_________________________
	|  _____  |1 ___________  |
	| |2___ | | |4_________ | |
	| ||3_ || | ||5        || |
	| |||9||| | ||_________|| |
	| ||| ||| | |___________| |	
	| ||| ||| |  ___________  |
	| ||| ||| | |6          | |	
	| ||| ||| | |___________| |
	| ||| ||| |  ___________  |
	| |||_||| | |7__________| |	
	| ||___|| |  ___________  |
	| |_____| | |8__________| |	
	|_________|_______________|		

    */

    GtkWidget *mainbox, *globalbox;
    GtkWidget *targetbox, *targetbox_scroll;
    GtkWidget *menubox, *menubox_vpad, *menubox_scroll;
    GtkWidget *actionbox, *infobox;
    GtkWidget *button_next, *button_prev;
    GtkWidget *info_frame, *progress_bar_frame;
    GtkWidget *view;
    GtkTextBuffer *buffer;
    GtkWidget *progress_bar;
	
    int *ret_val;

    mainbox = gtk_vbox_new (FALSE, 10);
    gtk_container_set_border_width (GTK_CONTAINER(mainbox), 5);

    /* This is the set of boxes and the containing viewport where the
     * questions will be displayed
     */
    targetbox = gtk_vbox_new (FALSE, 10);
    ((struct frontend_data*) obj->data)->target_box = targetbox;

    targetbox_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (targetbox_scroll), targetbox);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (targetbox_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    /* Here are the back and forward buttons */
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
    g_signal_connect (G_OBJECT(button_next), "clicked",
                      G_CALLBACK(exit_button_callback), obj);
    gtk_box_pack_start (GTK_BOX(actionbox), button_next, TRUE, TRUE, 2);
    GTK_WIDGET_SET_FLAGS (button_next, GTK_CAN_DEFAULT);

    ((struct frontend_data*) obj->data)->button_prev = button_prev;
    ((struct frontend_data*) obj->data)->button_next = button_next;


    /* Here a frame is created to display extended descriptions about the
     * questions
     */
    view = gtk_text_view_new ();
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    gtk_text_buffer_set_text (buffer, "Here you can get help upon installing Debian/GNU Linux", -1);
    gtk_text_view_set_editable (GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin (GTK_TEXT_VIEW(view), 5);
    gtk_text_view_set_right_margin (GTK_TEXT_VIEW(view), 5);
    ((struct frontend_data*) obj->data)->info_box = view;
    info_frame = gtk_frame_new("Help box");
    gtk_container_add(GTK_CONTAINER (info_frame), view);
    infobox = gtk_vbox_new (FALSE, 10);
    gtk_box_pack_start(GTK_BOX (infobox), info_frame, TRUE, TRUE, 5);

    /* A progress bar is created here; it will probably be removed from here
     * in the future and placed somewhere else (maybe inside a popup window?)
     */
    obj->progress_title = NULL;
    obj->progress_min = 0;
    obj->progress_max = 100;
    obj->progress_cur = 0;
    progress_bar = gtk_progress_bar_new ();
    ((struct frontend_data*)obj->data)->progress_bar = progress_bar;
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), " ");
	progress_bar_frame = gtk_frame_new("Debian-Installer progressbar");
    ((struct frontend_data*)obj->data)->progress_bar_frame = progress_bar_frame;
    gtk_container_add(GTK_CONTAINER (progress_bar_frame), progress_bar);

    /* This is where the main-menu will be displayed, in the left-area of
     * the screen
     */
    if ( ((struct frontend_data*)obj->data)->main_menu_enabled == TRUE)
	{
		menubox_vpad = gtk_vbox_new (FALSE, 10);
	    menubox = gtk_vbox_new (FALSE, 10);
	    ((struct frontend_data*) obj->data)->menu_box = menubox;
	    menubox_scroll = gtk_scrolled_window_new(NULL, NULL);
	    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (menubox_scroll), menubox);
	    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (menubox_scroll),
	                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (menubox_scroll), GTK_SHADOW_NONE);
		gtk_box_pack_start(GTK_BOX (menubox_vpad), menubox_scroll, TRUE, TRUE, 10);
		
	}

    /* Final packaging */
    gtk_box_pack_start(GTK_BOX (mainbox), targetbox_scroll, TRUE, TRUE, 5);	
    gtk_box_pack_start(GTK_BOX (mainbox), infobox, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX (mainbox), progress_bar_frame, FALSE, FALSE, 5);

    globalbox = gtk_hbox_new (TRUE, 10);
	if ( ((struct frontend_data*)obj->data)->main_menu_enabled == TRUE)
    	gtk_box_pack_start(GTK_BOX (globalbox), menubox_vpad, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX (globalbox), mainbox, TRUE, TRUE, 5);

    gtk_container_add(GTK_CONTAINER(window), globalbox);
}

static int gtk_initialize(struct frontend *obj, struct configuration *conf)
{
    struct frontend_data *fe_data;
    GtkWidget *window;
    int args = 1;
    char **name;

    name = malloc(2 * sizeof(char*));
    name[0] = strdup("debconf");
    name[1] = NULL;

    INFO(INFO_DEBUG, "GTK_DI - gtk_initialize() called");
    obj->data = NEW(struct frontend_data);
    obj->interactive = 1;

    /* Here we setup in the frontend structure the fields needed for the
     * mechanism that lets the user jump across questions to work
     */
    fe_data = obj->data;
    fe_data->window = NULL;
    fe_data->target_box = NULL;
    fe_data->info_box = NULL;
    fe_data->menu_box = NULL;
    fe_data->button_next = NULL;
    fe_data->button_prev = NULL;
    fe_data->progress_bar = NULL;
    fe_data->progress_bar_menubox = NULL;
    fe_data->progress_bar_frame = NULL;
    fe_data->setters = NULL;
    fe_data->button_val = DC_NOTOK;
    fe_data->ask_jump_confirmation = FALSE;
    *fe_data->jump_target = '\0';
    fe_data->q_main = NULL;
	
	/* Set this to TRUE/FALSE to enable/disable the main-menu hack
	 */
	fe_data->main_menu_enabled = TRUE;

    /* If fe_data->main_menu_enabled is set to TRUE, then this filed
     * has to match the tag of the question that is used as main-menu.
     * Set this to "debian-installer/main-menu" for use in the debian installer
     * or "test/select" to use the frontend with the testscripts that come with 
     * cdebconf sources.
     */     
	/* fe_data->main_menu_tag = strdup("test/select"); */
	fe_data->main_menu_tag = strdup("debian-installer/main-menu");

    gtk_init (&args, &name);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    set_window_properties (window);
    set_design_elements (obj, window);
    ((struct frontend_data*) obj->data)->window = window;
    gtk_widget_show_all(window);

    return DC_OK;
}

static void gtk_plugin_destroy_notify(void *data)
{
    plugin_delete((struct plugin *) data);
}

static int gtk_go(struct frontend *obj)
{
    struct frontend_data *data = (struct frontend_data *) obj->data;
    struct question *q = obj->questions;
    GtkWidget *questionbox, *menubox;
    GtkWidget *image_button_forward, *image_button_back;
    di_slist *plugins;
    int i, j;
    int ret;
    GtkWidget *helpbox_view;
    GtkTextBuffer *helpbox_buffer;

	/* Users's jumps do not need to be confirmated unless he has activated a widget */
	data->ask_jump_confirmation = FALSE;
	data->setters = NULL;

	helpbox_view = (GtkWidget*)data->info_box;
    helpbox_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (helpbox_view) );

    if (q == NULL) return DC_OK;


	if (((struct frontend_data*)obj->data)->main_menu_enabled == TRUE)
	{
	    /* The main-menu question is stored in a private area of the frontend,
	     * so that it can be shown even if it's not passed to the frontend
	     */
	    if (strcmp(q->tag, data->main_menu_tag) == 0)
	    {
	        if (data->q_main)
	            question_delete(data->q_main);
	        data->q_main = question_dup(q);
	        INFO(INFO_DEBUG, "GTK_DI - gtk_go() main question \"%s\" stored in memory", data->main_menu_tag);
	    }

	    /* this piece of code implements the "jump" mechanism: if the "jump_target"
	     * string is not empty it means that a jump was previously programmed
	     * to be executed: we must tell cdebconf to go back until we reach the main menu,
	     * only then the "jump" can be performed
	     */
	    if (strcmp(data->jump_target, "") != 0)
	    {
	        if (strcmp(q->tag, data->main_menu_tag) == 0)
	        {
	            /* the d-i has eventually just told us to show the main menu: now
	             * the jump can be executed (basically we simulate the users's click
	             * on the programmed jump target)
	             */
	            INFO(INFO_DEBUG, "GTK_DI - gtk_go() jumping to \"%s\"", data->jump_target);
	            q = obj->questions;
	            question_setvalue(q, data->jump_target);
	            obj->qdb->methods.set(obj->qdb, q);
	            q->next=NULL;
	            
	            /* once the jump is set we must reset the "jump_target" string */
	            strcpy(data->jump_target,"");

	            data->button_val = DC_OK;

	            return DC_OK;
	        }
	        else
	        {
	        	/* A jump is awaiting to be executed but the frontend was told to
	        	 * display something other than the main-menu: we must tell cdebconf
	        	 * to go back until it tells the frontend to display the main-menu:
	        	 * only then the jump will be executed
	        	 */
	            data->button_val = DC_GOBACK;

	            INFO(INFO_DEBUG, "GTK_DI - gtk_go() backing up to jump to \"%s\"", data->jump_target);

	            return DC_GOBACK;
	        }
	    }


		/* The dummy menubox should be destroyed by gtk_progressbar_stop() but
		 * since sometimes this function is not called we make sure the dummy
		 * menubox has been destroyed before displaying the real one
		 */	
		if(data->progress_bar_menubox != NULL)
			{
			gtk_widget_destroy(GTK_WIDGET(data->progress_bar_menubox));
			data->progress_bar_menubox = NULL;
			}

	    menubox = gtk_vbox_new(FALSE, 5);
	    gtk_box_pack_start(GTK_BOX(data->menu_box), menubox, FALSE, FALSE, 5);
	    
	    if (strcmp (q->tag, data->main_menu_tag) == 0)
	    {
	        /* the first question in the question list is the main menu, so we
	         * simply handle it
	         */
	        ret = gtkhandler_main_menu(obj, q, menubox, TRUE, FALSE);
	        gtk_text_buffer_set_text (helpbox_buffer, q_get_extended_description(q), -1);
	        q = q->next; 
	    }
	    else if (data->q_main)
	    {
	        /* the first question in the question list is not the main menu, so
	         * we need to show it using the copy of the main menu stored
	         * previously into memory.
	         * If the first real question passed to the frontend doesn't alow to go
	         * back we must prevent the user from jumping, since juping consists
	         * of a sequence of "back" simulated commands
	         */
	        if ( obj->methods.can_go_back(obj, q) )
		        ret = gtkhandler_main_menu(obj, data->q_main, menubox,TRUE ,TRUE);
	      	else
		        ret = gtkhandler_main_menu(obj, data->q_main, menubox,FALSE ,TRUE);
	    }
	}
	
    questionbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(data->target_box), questionbox, FALSE, FALSE, 5);

    /* now we can safely handle all other questions, if any */
    j = 0;
    plugins = di_slist_alloc();
    while (q != NULL)
    {
    	j++;
    	INFO(INFO_DEBUG, "GTK_DI - question %d: %s (type %s)", j, q->tag, q->template->type);
        for (i = 0; i < DIM(question_handlers); i++)
        {
            gtk_handler *handler;
            struct plugin *plugin = NULL;

            if (*question_handlers[i].type)
                handler = question_handlers[i].handler;
            else {
                plugin = plugin_find(obj, q->template->type);
                if (plugin) {
                    INFO(INFO_DEBUG, "GTK_DI - Found plugin for %s", q->template->type);
                    handler = (gtk_handler *) plugin->handler;
                    di_slist_append(plugins, plugin);
                } else {
                    INFO(INFO_DEBUG, "GTK_DI - No plugin for %s", q->template->type);
                    continue;
                }
            }

            if (plugin || strcmp(q->template->type, question_handlers[i].type) == 0)
            {
                ret = handler(obj, q, questionbox);
                if (ret != DC_OK)
                {
                    di_slist_destroy(plugins, &gtk_plugin_destroy_notify);
                    INFO(INFO_DEBUG, "GTK_DI - question %d: \"%s\" failed to display!", j, q->tag);
                }
				else
				{
					/* The (extended) description of the last handled question is
					 * automatically displayed inside helpbox to give
					 * the user better help
					 */
				    if (strlen(q_get_extended_description(q)) > 0)
				    	gtk_text_buffer_set_text (helpbox_buffer, q_get_extended_description(q), -1);
				    else if (strlen(q_get_description(q)) > 0)
				    	gtk_text_buffer_set_text (helpbox_buffer, q_get_description(q), -1); 	
				}
                /* we've found the right handler for the question, so we break
                 * the for() loop
                 */
                break;
            }
        }

        q = q->next;
    }

    if ( obj->methods.can_go_back(obj, q) )
        gtk_widget_set_sensitive (data->button_prev, TRUE);
    else
        gtk_widget_set_sensitive (data->button_prev, FALSE);

    gtk_widget_set_sensitive(GTK_WIDGET(data->button_next), TRUE);

	gtk_button_set_label (GTK_BUTTON(data->button_prev), get_text(obj, "debconf/button-goback", "Go Back") );
	gtk_button_set_label (GTK_BUTTON(data->button_next), get_text(obj, "debconf/button-continue", "Continue") );
	image_button_back = gtk_image_new_from_stock (GTK_STOCK_GO_BACK, GTK_ICON_SIZE_SMALL_TOOLBAR );
	image_button_forward = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR );

	/* gtk_button_set_image() was first implemented in GTK v. 2.6 while
	 * the most recent working build of GTK with GDK DFB support is 2.0.9,
	 * so we'll have to stay with text-only arrows until GDK DFB library
	 * is update to compile with GTK v 2.8 :(
     */
     	
	/* gtk_button_set_image (GTK_BUTTON(data->button_prev), image_button_back); */    
	/* gtk_button_set_image (GTK_BUTTON(data->button_next), image_button_forward); */

    
    gtk_widget_set_default_direction(get_text_direction(obj));

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

    di_slist_destroy(plugins, &gtk_plugin_destroy_notify);
    gtk_widget_destroy(questionbox);
    
    if (((struct frontend_data*)obj->data)->main_menu_enabled == TRUE)
    	gtk_widget_destroy(menubox);

	gtk_widget_set_sensitive (data->button_prev, FALSE);
	gtk_widget_set_sensitive (data->button_next, FALSE);

    if (data->button_val == DC_OK)
        return DC_OK;
    else if (data->button_val == DC_GOBACK)
        return DC_GOBACK;
    else
        return DC_OK;
}

static bool gtk_can_go_back(struct frontend *obj, struct question *q)
{
    return (obj->capability & DCF_CAPB_BACKUP);
}

/* If the main_menu is enabled then it has to be displayed with unsensitive
 * buttons while the progressbar runs
 */
static void display_main_menu_while_progressbar_runs(struct frontend *obj)
{
	struct frontend_data *data;
	GtkWidget *menubox;
	
	data=obj->data;
	/* before displaying a ghosted main menu we make sure another one
	 * is not already displayed
	 */
	if( data->progress_bar_menubox != NULL ) return;
    menubox = gtk_vbox_new(FALSE, 5);
    data->progress_bar_menubox=menubox;
    gtk_box_pack_start(GTK_BOX(data->menu_box), menubox, FALSE, FALSE, 5);
    /* TODO: q_main isn't set if we're in a progress bar at high priority
     * (i.e. main-menu not displayed) and no questions have been asked yet.
     * We probably ought to do something sensible in that case, but I don't
     * know what. Attilio, please investigate.
     */
    if (data->q_main)
    	gtkhandler_main_menu(obj, data->q_main, menubox,FALSE ,FALSE);
    	
    gtk_widget_show_all(data->window);
}

static void gtk_progress_start(struct frontend *obj, int min, int max, const char *title)
{
    GtkWidget *progress_bar, *progress_bar_frame;

	if ( ((struct frontend_data*)obj->data)->main_menu_enabled == TRUE)
		display_main_menu_while_progressbar_runs(obj);
    
    progress_bar = ((struct frontend_data*)obj->data)->progress_bar;
    progress_bar_frame = ((struct frontend_data*)obj->data)->progress_bar_frame;
    DELETE(obj->progress_title);
    obj->progress_title=strdup(title);
    gtk_frame_set_label( GTK_FRAME(progress_bar_frame) , obj->progress_title );
    obj->progress_min = min;
    obj->progress_max = max;
    obj->progress_cur = min;

    INFO(INFO_DEBUG, "GTK_DI - gtk_progress_start(min=%d, max=%d, title=%s) called", min, max, title);

    while (gtk_events_pending ())
	gtk_main_iteration ();
}

static void gtk_progress_set(struct frontend *obj, int val)
{
    gdouble progress;
    GtkWidget *progress_bar;

	if ( ((struct frontend_data*)obj->data)->main_menu_enabled == TRUE)
		display_main_menu_while_progressbar_runs(obj);

    INFO(INFO_DEBUG, "GTK_DI - gtk_progress_set(val=%d) called", val);

    progress_bar = ((struct frontend_data*)obj->data)->progress_bar;

    obj->progress_cur = val;
    if ((obj->progress_max - obj->progress_min) > 0)
    {
        progress = (gdouble)(obj->progress_cur - obj->progress_min) /
                   (gdouble)(obj->progress_max - obj->progress_min);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), progress);
    }

    while (gtk_events_pending ())
        gtk_main_iteration ();
}

static void gtk_progress_info(struct frontend *obj, const char *info)
{
    GtkWidget *progress_bar;

	if ( ((struct frontend_data*)obj->data)->main_menu_enabled == TRUE)
		display_main_menu_while_progressbar_runs(obj);

    INFO(INFO_DEBUG, "GTK_DI - gtk_progress_info(%s) called", info);

    progress_bar = ((struct frontend_data*)obj->data)->progress_bar;
      
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), info);
   
    while (gtk_events_pending ())
        gtk_main_iteration ();
}

static void gtk_progress_stop(struct frontend *obj)
{
    GtkWidget *progress_bar, *progress_bar_frame;
#if 0
    struct frontend_data *data = (struct frontend_data *) obj->data;
#endif

    /* Altough logically correct, for cosmetic reasons it's better
     * not to destroy the ghosted main menu until a question is
     * asked to the user
     */
#if 0
    gtk_widget_destroy(GTK_WIDGET(data->progress_bar_menubox));
    data->progress_bar_menubox = NULL;
#endif
    
    progress_bar = ((struct frontend_data*)obj->data)->progress_bar;
    progress_bar_frame = ((struct frontend_data*)obj->data)->progress_bar_frame;
    
    INFO(INFO_DEBUG, "GTK_DI - gtk_progress_stop() called");
    /* gtk_widget_destroy(gtk_widget_get_parent(((struct frontend_data*)obj->data)->progress_bar)); */
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), " " );
	gtk_frame_set_label( GTK_FRAME(progress_bar_frame) , "Debian-Installer progressbar" );
    gtk_widget_show_all(((struct frontend_data*)obj->data)->window);
    while (gtk_events_pending ())
        gtk_main_iteration ();
}

static unsigned long gtk_query_capability(struct frontend *f)
{
    INFO(INFO_DEBUG, "GTK_DI - gtk_query_capability() called");
    return DCF_CAPB_BACKUP;
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
    query_capability: gtk_query_capability,
};
