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

#include <syslog.h>
#include <debian-installer/slist.h>
#include <gdk/gdkkeysyms.h>

#if GTK_CHECK_VERSION(2,10,0)
#ifdef GDK_WINDOWING_DIRECTFB
#include <directfb.h>
#endif
#else
#include <directfb.h>
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

/* maximum length for string questions */
#define STRING_MAX_LENGTH 128

/* used to define horizontal and vertical padding of progressbar */
#define PROGRESSBAR_HPADDING 60
#define PROGRESSBAR_VPADDING 60

typedef int (gtk_handler)(struct frontend *obj, struct question *q, GtkWidget *questionbox);

static GCond *button_cond = NULL;
static GMutex *button_mutex = NULL;

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

static const char * get_text(struct frontend *obj, const char *template, const char *fallback );

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

void free_treemodel_data( GtkWidget *widget, struct question_treemodel_data* data )
{
    free(data);
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

gboolean expose_event_callback(GtkWidget *wid, GdkEventExpose *event, struct frontend *obj)
{
    PangoLayout *layout; 
    gint w, h;
    char *message;

    if (obj->info != NULL) {
        char *text = q_get_description(obj->info);
        if (text) {
            /* setting font colour makes GTKDFB 2.0.9 crash */
            #if GTK_CHECK_VERSION(2,8,0)
            message = malloc(strlen(text) + 42 );
            sprintf(message,"<b><span foreground=\"#ffffff\">%s</span></b>", text);
            #else
            message = malloc(strlen(text) + 8 );
            sprintf(message,"<b>%s</b>", text);
            #endif
            layout = gtk_widget_create_pango_layout(wid, NULL);
            pango_layout_set_markup(layout, message, strlen(message));
            pango_layout_set_font_description(layout, pango_font_description_from_string("Sans 12"));
            pango_layout_get_pixel_size(layout, &w, &h);
            /* obj->info is drawn over the debian banner, top-right corner of the screen */
            gdk_draw_layout(wid->window, gdk_gc_new(wid->window),  WINDOW_WIDTH - w - 4, 4, layout);
            free(message);
        }
        free(text);
    }
    return FALSE;
}

void screenshot_button_callback(GtkWidget *button, struct frontend* obj )
{
    GdkWindow *gdk_window;
    GdkPixbuf *gdk_pixbuf;
    GtkWidget *window, *frame, *message_label, *title_label, *h_box, *v_box, *v_box_outer, *close_button, *actionbox, *separator;
    gint x, y, width, height, depth;
    int i, j;
    char screenshot_name[256], popup_message[256];
    char *label_title_string;
	
    gdk_window = gtk_widget_get_parent_window ( button );
    gdk_window_get_geometry( gdk_window, &x, &y, &width, &height, &depth);
    gdk_pixbuf = gdk_pixbuf_get_from_drawable(NULL, gdk_window, gdk_colormap_get_system(),0,0,0,0, width, height);
    i=0;
	while (TRUE) {
        sprintf(screenshot_name, "%s_%d.png", (obj->questions)->tag, i );
        for(j=0; j<strlen(screenshot_name); j++) {
	        if (screenshot_name[j] == '/')
	            screenshot_name[j] = '_';
        }
        sprintf(popup_message, "/var/log/%s", screenshot_name );
        sprintf(screenshot_name, "%s", popup_message );
        if ( ! access(screenshot_name, R_OK) )
            i++;
        else {
            /* printf ("name: %s.png\nx: %d\ny: %d\nwidth: %d\nheight: %d\ndepth=%d\n", screenshot_name, x, y, width, height, depth); */
            break;
        }
    }
    gdk_pixbuf_save (gdk_pixbuf, screenshot_name, "png", NULL, NULL);
    g_object_unref(gdk_pixbuf);

    /* A message inside a popup window tells the user the sceenshot has
     * been saved correctly
     */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (window), 0);

    title_label = gtk_label_new (get_text(obj, "debconf/gtk-button-screenshot", "Screenshot"));
    gtk_misc_set_alignment(GTK_MISC(title_label), 0, 0);
    label_title_string = malloc(strlen(get_text(obj, "debconf/gtk-button-screenshot", "Screenshot")) + 8 );
    sprintf(label_title_string,"<b>%s</b>", get_text(obj, "debconf/gtk-button-screenshot", "Screenshot"));
    gtk_label_set_markup(GTK_LABEL(title_label), label_title_string);
    sprintf(popup_message, get_text(obj, "debconf/gtk-screenshot-saved", "Screenshot saved as %s"), screenshot_name );
    message_label = gtk_label_new (popup_message);

    actionbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout (GTK_BUTTON_BOX(actionbox), GTK_BUTTONBOX_END);
    close_button = gtk_button_new_with_label (get_text(obj, "debconf/button-continue", "Continue"));
    g_signal_connect_swapped (G_OBJECT (close_button), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (window));
    gtk_box_pack_end (GTK_BOX(actionbox), close_button, TRUE, TRUE, DEFAULT_PADDING);

    v_box = gtk_vbox_new(FALSE, DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX (v_box), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (v_box), message_label, FALSE, FALSE, DEFAULT_PADDING);
    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX (v_box), separator, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (v_box), actionbox, FALSE, FALSE, 0);
    h_box = gtk_hbox_new(FALSE, DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX (h_box), v_box, FALSE, FALSE, DEFAULT_PADDING);
    v_box_outer = gtk_vbox_new(FALSE, DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX (v_box_outer), h_box, FALSE, FALSE, DEFAULT_PADDING);
    
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_OUT);
    gtk_container_add (GTK_CONTAINER (frame), v_box_outer);
    gtk_container_add (GTK_CONTAINER (window), frame);
    gtk_widget_show_all (window);

    free(label_title_string);
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

    /* INFO(INFO_DEBUG, "GTK_DI - check_toggled_callback() called"); */
    value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(toggle));
    bool_setter (toggle, q);
}

void boolean_single_callback(GtkWidget *button, struct frontend_question_data* data )
{
    struct frontend *obj = data->obj;
    struct question *q = data->q;
    char *ret;

    /* INFO(INFO_DEBUG, "GTK_DI - boolean_single_callback() called"); */
    ret = (char*) gtk_object_get_user_data(GTK_OBJECT(button));
    question_setvalue(q, ret);
    free(ret);

    ((struct frontend_data*)obj->data)->button_val = DC_OK;;

    gtk_main_quit();
}

void multiselect_single_callback(GtkCellRendererToggle *cell, const gchar *path_string, struct question_treemodel_data* data)
{
    int i, count;
    char **choices, **choices_translated;
    int *tindex = NULL;
    gchar *indices;
    gchar *result = NULL, *copy = NULL ;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gboolean bool_var ;
    struct question *q = data->q;
    model = (GtkTreeModel *) data->treemodel;
    char str_iter_index[4];

    /* GtkTreeView is updated */
    path  = gtk_tree_path_new_from_string(path_string);
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);
    gtk_tree_model_get(model, &iter, MULTISELECT_COL_BOOL, &bool_var, -1);
    bool_var ^= 1;
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, MULTISELECT_COL_BOOL, bool_var, -1);

    /* frontend's internal question struct is updated */
    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0)
        return /* DC_NOTOK */;
    choices = malloc(sizeof(char *) * count);
    choices_translated = malloc(sizeof(char *) * count);
    tindex = malloc(sizeof(int) * count);
       indices = q_get_indices(q);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
        return /* DC_NOTOK */;


    for (i = 0; i < count; i++)
    {
        sprintf(str_iter_index, "%d", i);
        path  = gtk_tree_path_new_from_string(str_iter_index);

        /* TODO
         * The above hack is needed since gtk_tree_path_new_from_indices()
         * was implemented first in GTK v. 2.2
         * path  = gtk_tree_path_new_from_indices ( i, -1);
         */
        gtk_tree_model_get_iter(model, &iter, path);
        gtk_tree_path_free(path);
        gtk_tree_model_get(model, &iter, MULTISELECT_COL_BOOL, &bool_var, -1);

        if((result != NULL) && bool_var==1)
        {
            copy = g_strdup(result);
            free(result);
            result = g_strconcat(copy, ", ", choices[tindex[i]], NULL);
            free(copy);
        }
        else if((result == NULL) && bool_var==1)
            result = g_strdup(choices[tindex[i]]);
    }

    if (result == NULL)
        result = g_strdup("");

    question_setvalue(q, result);
    free(result);
    free(choices);
    free(choices_translated);
    free(tindex);
    free(indices);

}

static gboolean key_press_event( GtkWidget *widget, GdkEvent  *event, struct frontend* obj )
{
    GdkEventKey* key = (GdkEventKey*)event;
    struct frontend_data *data = (struct frontend_data *) obj->data;
    struct question *q = obj->questions;
    
    if ( (key->keyval  == GDK_Escape) && (obj->methods.can_go_back(obj, q)) ) {
        /* INFO(INFO_DEBUG, "GTK_DI - ESC key pressed\n"); */
        gtk_button_clicked ( GTK_BUTTON(data->button_prev) );
    }

    return TRUE;
}

void exit_button_callback(GtkWidget *button, struct frontend* obj)
{
    int value;
    void *ret;

    ret = gtk_object_get_user_data(GTK_OBJECT(button));
    value = *(int*) ret;

    /* INFO(INFO_DEBUG, "GTK_DI - exit_button_callback() called, value: %d", value); */

    ((struct frontend_data*)obj->data)->button_val = value;

    g_mutex_lock (button_mutex);
    /* gtk_go() gets unblocked */ 
    g_cond_signal (button_cond);
    g_mutex_unlock (button_mutex);
}

void cancel_button_callback(GtkWidget *button, struct frontend* obj)
{
    ((struct frontend_data*)obj->data)->button_val = DC_GOBACK;
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

    /* INFO(INFO_DEBUG, "GTK_DI - gboolean select_treeview_callback() called"); */

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
        gtk_tree_model_get(model, &iter, SELECT_COL_NAME, &name, -1);
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

/* catches double-clicks, SPACEBAR, ENTER keys pressure for SELECT questions */
void
select_onRowActivated (GtkTreeView          *treeview,
                       GtkTreePath          *path,
                       GtkTreeViewColumn    *col,
                       struct frontend_data *data)
{
    gtk_button_clicked ( GTK_BUTTON( data->button_next ) );
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

GtkWidget* display_descriptions(struct question *q, struct frontend *obj)
{
    GtkWidget *description_view, *ext_description_view;
    GtkWidget *returned_box, *description_box, *icon_box, *icon_button;
    GtkTextBuffer *description_buffer, *ext_description_buffer;
    GdkColor *bg_color;
    GtkTextIter start, end;
    GtkStyle *style;

    style = gtk_widget_get_style (((struct frontend_data*)obj->data)->window);
    bg_color = style->bg;

    description_box = gtk_vbox_new (FALSE, 0);
    icon_box = gtk_vbox_new (FALSE, 0);
    returned_box = gtk_hbox_new (FALSE, 0);

    /* here is created the question's extended description, but only
     * if the question's extended description actually exists
     */
    if (strlen (q_get_extended_description(q)) > 0)
    {
        ext_description_view = gtk_text_view_new ();
        ext_description_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (ext_description_view));
        gtk_text_buffer_set_text (ext_description_buffer, q_get_extended_description(q), -1);
        gtk_text_view_set_editable (GTK_TEXT_VIEW(ext_description_view), FALSE);
        gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(ext_description_view), FALSE);
        gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(ext_description_view), GTK_WRAP_WORD);
        gtk_widget_modify_base(GTK_WIDGET(ext_description_view), GTK_STATE_NORMAL, bg_color);
    }

    /* here is created the question's description, unless question is BOOLEAN */
    if( strcmp(q->template->type, "boolean") != 0 ) {
        description_view = gtk_text_view_new ();
        description_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (description_view));
        gtk_text_buffer_set_text (description_buffer, q_get_description(q), -1);
        gtk_text_view_set_editable (GTK_TEXT_VIEW(description_view), FALSE);
        gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(description_view), FALSE);
        gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(description_view), GTK_WRAP_WORD);
        gtk_text_view_set_left_margin (GTK_TEXT_VIEW(description_view), 4);
        gtk_text_view_set_right_margin (GTK_TEXT_VIEW(description_view), 4);
        gtk_text_buffer_create_tag (description_buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);
        g_object_set_data (G_OBJECT (description_view), "tag", "italic");
        gtk_text_buffer_get_start_iter  (description_buffer, &start);
        gtk_text_buffer_get_end_iter  (description_buffer, &end);
        gtk_text_buffer_apply_tag_by_name (description_buffer, "italic", &start, &end);
        gtk_widget_modify_base(GTK_WIDGET(description_view), GTK_STATE_NORMAL, bg_color);
    }

    gtk_container_set_focus_chain(GTK_CONTAINER(description_box), NULL);

    if ( (strcmp(q->template->type,"note") == 0) || (strcmp(q->template->type,"error") == 0) )
    {
        gtk_box_pack_start(GTK_BOX (description_box), description_view, FALSE, FALSE, 3);
        if (strlen (q_get_extended_description(q)) > 0)
            gtk_box_pack_start(GTK_BOX (description_box), ext_description_view, FALSE, FALSE, 2);
    }
    else
    {
        if (strlen (q_get_extended_description(q)) > 0)
            gtk_box_pack_start(GTK_BOX (description_box), ext_description_view, FALSE, FALSE, 2);
        if( strcmp(q->template->type, "boolean") != 0 )
            gtk_box_pack_start(GTK_BOX (description_box), description_view, FALSE, FALSE, 3);
    }

    if ( strcmp(q->template->type,"note") == 0 )
    {
        icon_button = gtk_image_new_from_file("/usr/share/graphics/note_icon.png");
        gtk_box_pack_start(GTK_BOX (icon_box), icon_button, FALSE, FALSE, 3);
        gtk_box_pack_start(GTK_BOX (returned_box), icon_box, FALSE, FALSE, 3);
    }
    else if( strcmp(q->template->type,"error") == 0 )
    {
        icon_button = gtk_image_new_from_file("/usr/share/graphics/warning_icon.png");
        gtk_box_pack_start(GTK_BOX (icon_box), icon_button, FALSE, FALSE, 3);
        gtk_box_pack_start(GTK_BOX (returned_box), icon_box, FALSE, FALSE, 3);
    }

    gtk_box_pack_start(GTK_BOX (returned_box), description_box, TRUE, TRUE, 3);
    return returned_box;
}

static int gtkhandler_boolean(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *description_box, *check, *hpadbox, *vpadbox;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_boolean() called"); */

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    check = gtk_check_button_new_with_label(q_get_description(q));
    if (strcmp(defval, "true") == 0)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);

    if (q->next == NULL && q->prev == NULL)
        g_signal_connect (G_OBJECT(check), "toggled", G_CALLBACK(check_toggled_callback), q);
    else
        register_setter(bool_setter, check, q, obj);

    g_signal_connect (G_OBJECT(check), "destroy", G_CALLBACK (free_description_data), data);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(vpadbox), description_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(vpadbox), check, FALSE, FALSE, 0);
    hpadbox = gtk_hbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(hpadbox), vpadbox, TRUE, TRUE, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE, FALSE, QUESTIONBOX_VPADDING);

    if (is_first_question(q))
        gtk_widget_grab_focus(check);

    return DC_OK;
}

static int gtkhandler_multiselect_single(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *description_box, *hpadbox, *vpadbox;
    char **choices, **choices_translated, **defvals;
    int i, j, count, defcount;
    struct question_treemodel_data *data;
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);

    GtkTreeModel        *model;
    GtkListStore        *store;
    GtkTreeIter          iter;
    GtkWidget           *view, *scroll, *frame;
    GtkCellRenderer     *renderer, *renderer_check;
    GtkTreePath         *path;

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

    view = gtk_tree_view_new ();
    gtk_tree_view_set_headers_visible ( GTK_TREE_VIEW (view), FALSE);
    store = gtk_list_store_new (MULTISELECT_NUM_COLS, G_TYPE_INT, G_TYPE_STRING );

    renderer_check = gtk_cell_renderer_toggle_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, NULL, renderer_check, "active", MULTISELECT_COL_BOOL, NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), -1, NULL, renderer, "text", MULTISELECT_COL_NAME, NULL);

    model = GTK_TREE_MODEL( store );
    gtk_tree_view_set_model (GTK_TREE_VIEW (view), model);
    data = NEW(struct question_treemodel_data);
    data->q = q;
    data->treemodel = model;
    g_signal_connect(G_OBJECT(renderer_check), "toggled", G_CALLBACK(multiselect_single_callback), data);
    g_signal_connect (G_OBJECT(view), "destroy", G_CALLBACK (free_treemodel_data), data);
    g_object_unref (model);

    for (i = 0; i < count; i++)
    {
        gtk_list_store_append (store, &iter);

        if (defcount == 0)
            gtk_list_store_set (store, &iter, MULTISELECT_COL_BOOL, FALSE, MULTISELECT_COL_NAME, choices_translated[i],  -1);

        else
        {
            for (j = 0; j < defcount; j++)
            {
                if (strcmp(choices[tindex[i]], defvals[j]) == 0)
                {
                    gtk_list_store_set (store, &iter, MULTISELECT_COL_BOOL, TRUE, MULTISELECT_COL_NAME, choices_translated[i],  -1);
                    break;
                }
                else
                {
                    gtk_list_store_set (store, &iter, MULTISELECT_COL_BOOL, FALSE, MULTISELECT_COL_NAME, choices_translated[i],  -1);
                }
            }
        }
        
		/* by default the first row gets selected if no default option is specified */
        gtk_tree_model_get_iter_first (model,&iter);
        path = gtk_tree_model_get_path (model, &iter);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW(view), path, MULTISELECT_COL_BOOL, FALSE);
        gtk_tree_path_free (path);

        free(choices[tindex[i]]);
        free(choices_translated[i]);
    }

    free(choices);
    free(choices_translated);
    free(tindex);
    for (j = 0; j < defcount; j++)
        free(defvals[j]);
    free(defvals);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(vpadbox), description_box, FALSE, FALSE, 0);
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), scroll);
    gtk_box_pack_start (GTK_BOX(vpadbox), frame, TRUE, TRUE, 0);

    hpadbox = gtk_hbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(hpadbox), vpadbox, TRUE, TRUE, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE, TRUE, QUESTIONBOX_VPADDING);

    gtk_widget_grab_focus(view);

    return DC_OK;
}

static int gtkhandler_multiselect_multiple(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *description_box, *check_container, *check, *hpadbox, *vpadbox;
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

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(vpadbox), description_box, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(vpadbox), check_container, TRUE, TRUE, 0);
    hpadbox = gtk_hbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(hpadbox), vpadbox, TRUE, TRUE, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE, FALSE, QUESTIONBOX_VPADDING);

    register_setter(multi_setter, check_container, q, obj);

    return DC_OK;
}


static int gtkhandler_multiselect(struct frontend *obj, struct question *q, GtkWidget *qbox)
{

    if (q->prev == NULL && q->next == NULL)
           return gtkhandler_multiselect_single(obj, q, qbox);
    else
        return gtkhandler_multiselect_multiple(obj, q, qbox);
}

static int gtkhandler_note(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *hpadbox, *vpadbox, *description_box;

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_note() called"); */

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(vpadbox), description_box, FALSE, FALSE, 0);
    hpadbox = gtk_hbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(hpadbox), vpadbox, TRUE, TRUE, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE, FALSE, QUESTIONBOX_VPADDING);

    return DC_OK;
}

static int gtkhandler_text(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    return gtkhandler_note(obj, q, qbox);
}

static int gtkhandler_password(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *description_box, *entry, *hpadbox, *vpadbox;
    struct frontend_question_data *data;

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_password() called"); */

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry), STRING_MAX_LENGTH);
    gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

    g_signal_connect (G_OBJECT(entry), "destroy", G_CALLBACK (free_description_data), data);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(vpadbox), description_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(vpadbox), entry, FALSE, FALSE, 0);
    hpadbox = gtk_hbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(hpadbox), vpadbox, TRUE, TRUE, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE, FALSE, QUESTIONBOX_VPADDING);

    register_setter(entry_setter, entry, q, obj);

    if (is_first_question(q))
        gtk_widget_grab_focus(entry);

    return DC_OK;
}

static int gtkhandler_select_treeview_list(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    char **choices, **choices_translated;
    int i, count;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);
    GtkWidget *hpadbox, *vpadbox, *description_box;
    int flag_default_set = FALSE;

    GtkTreeModel        *model;
    GtkListStore        *store;
    GtkTreeIter          iter;
    GtkWidget           *view, *scroll, *frame;
    GtkCellRenderer     *renderer;
    GtkTreeSelection    *selection;
    GtkTreePath         *path;

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_select_treeview_list() called"); */

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
    gtk_tree_view_set_headers_visible ( GTK_TREE_VIEW (view), FALSE);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), -1, q_get_description(q), renderer, "text", SELECT_COL_NAME, NULL);
    store = gtk_list_store_new (SELECT_NUM_COLS, G_TYPE_STRING, G_TYPE_UINT);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    g_signal_connect (G_OBJECT(view), "row-activated", G_CALLBACK (select_onRowActivated), (struct frontend_data *) obj->data);
    gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc) select_treeview_callback, data, NULL);
    g_signal_connect (G_OBJECT(view), "destroy", G_CALLBACK (free_description_data), data);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW(view), TRUE);

    model = GTK_TREE_MODEL( store );
    gtk_tree_view_set_model (GTK_TREE_VIEW (view), model);
    g_object_unref (model);

    for (i = 0; i < count; i++)
    {
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, SELECT_COL_NAME, choices_translated[i], -1);
        if (defval && strcmp(choices[tindex[i]], defval) == 0)
        {
            path = gtk_tree_model_get_path(model, &iter);
            gtk_tree_view_scroll_to_cell    (GTK_TREE_VIEW(view), path, NULL, FALSE, 0.5, 0);
            gtk_tree_view_set_cursor        (GTK_TREE_VIEW(view), path, NULL, FALSE);
            gtk_tree_path_free (path);
            flag_default_set = TRUE;
        }
        free(choices[tindex[i]]);
    }
	/* by default the first row gets selected if no default option is specified */
	if( flag_default_set == FALSE )
	{
		gtk_tree_model_get_iter_first (model,&iter);
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW(view), path, NULL, FALSE);
		gtk_tree_path_free (path);
	}

    free(choices);
    free(choices_translated);
    free(tindex);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(vpadbox), description_box, FALSE, FALSE, 0);
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), scroll);
    gtk_box_pack_start (GTK_BOX(vpadbox), frame, TRUE, TRUE, 0);
    hpadbox = gtk_hbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(hpadbox), vpadbox, TRUE, TRUE, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE, TRUE, QUESTIONBOX_VPADDING);

    gtk_widget_grab_focus(view);

    return DC_OK;
}

/* some SELECT questions like "countrychooser/country-name" are
 * better displayed with a tree rather than a list and this question
 * handler is meant for this purpose
 */
static int gtkhandler_select_treeview_store(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    char **choices, **choices_translated;
    int i, count;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");
    int *tindex = NULL;
    const gchar *indices = q_get_indices(q);
    GtkWidget *hpadbox, *vpadbox, *description_box;

    GtkTreeModel        *model;
    GtkTreeStore        *store;
    GtkTreeIter          iter, child;
    GtkWidget           *view, *scroll, *frame;
    GtkCellRenderer     *renderer;
    GtkTreeSelection    *selection;
    GtkTreePath         *path;
	
    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_select_treeview_store() called"); */

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
    gtk_tree_view_set_headers_visible ( GTK_TREE_VIEW (view), FALSE);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), -1, q_get_description(q), renderer, "text", SELECT_COL_NAME, NULL);
    store = gtk_tree_store_new (SELECT_NUM_COLS, G_TYPE_STRING, G_TYPE_UINT);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc) select_treeview_callback, data, NULL);
    g_signal_connect (G_OBJECT(view), "row-activated", G_CALLBACK (select_onRowActivated), (struct frontend_data *) obj->data);
    g_signal_connect (G_OBJECT(view), "destroy", G_CALLBACK (free_description_data), data);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW(view), TRUE);
    model = GTK_TREE_MODEL( store );
    gtk_tree_view_set_model (GTK_TREE_VIEW (view), model);
    g_object_unref (model);

    for (i = 0; i < count; i++)
    {

        if(strcmp(q->tag, "countrychooser/country-name") == 0 )
        {
            if( ((choices_translated[i][0]=='-') && (choices_translated[i][1]=='-')) )
            {    /* father */
                gtk_tree_store_append (store, &iter,NULL);
                gtk_tree_store_set (store, &iter, SELECT_COL_NAME, choices_translated[i], -1);
            }
            else
            {    /* child */
                gtk_tree_store_append (store, &child, &iter);
                gtk_tree_store_set (store, &child, SELECT_COL_NAME, choices_translated[i], -1);

                if (defval && strcmp(choices[tindex[i]], defval) == 0)
                {
                    path = gtk_tree_model_get_path(model, &iter);
                    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(view), path, NULL, FALSE, 0.5, 0);
                    gtk_tree_view_expand_row (GTK_TREE_VIEW(view), path, TRUE);
                    gtk_tree_view_set_cursor (GTK_TREE_VIEW(view), path, NULL, FALSE);
                    gtk_tree_path_free (path);
                }
            }
        }
        else if(strcmp(q->tag, "partman/choose_partition") == 0 )
        {
            if( strstr(choices_translated[i],"    ")!=NULL )
            {    /* child */
                gtk_tree_store_append (store, &child, &iter);
                gtk_tree_store_set (store, &child, SELECT_COL_NAME, choices_translated[i], -1);

                path = gtk_tree_model_get_path(model, &iter);
                if (defval && strcmp(choices[tindex[i]], defval) == 0)
                {
                    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(view), path, NULL, FALSE, 0.5, 0);
                    gtk_tree_view_set_cursor (GTK_TREE_VIEW(view), path, NULL, FALSE);
                }
                path = gtk_tree_model_get_path(model, &iter);
                gtk_tree_view_expand_row (GTK_TREE_VIEW(view), path, TRUE);
                gtk_tree_path_free (path);
            }
            else
            {    /* father */
                gtk_tree_store_append (store, &iter,NULL);
                gtk_tree_store_set (store, &iter, SELECT_COL_NAME, choices_translated[i], -1);
                if (defval && strcmp(choices[tindex[i]], defval) == 0)
                {
                    path = gtk_tree_model_get_path(model, &iter);
                    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(view), path, NULL, FALSE, 0.5, 0);
                    gtk_tree_view_set_cursor (GTK_TREE_VIEW(view), path, NULL, FALSE);
                    gtk_tree_path_free (path);
                }
            }
        }
        free(choices[tindex[i]]);
    }

    free(choices);
    free(choices_translated);
    free(tindex);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(vpadbox), description_box, FALSE, FALSE, 0);
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), scroll);
    gtk_box_pack_start (GTK_BOX(vpadbox), frame, TRUE, TRUE, 0);
    hpadbox = gtk_hbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(hpadbox), vpadbox, TRUE, TRUE, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE, TRUE, QUESTIONBOX_VPADDING);

    gtk_widget_grab_focus(view);

    return DC_OK;
}

static int gtkhandler_select_multiple(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *description_box, *combo, *hpadbox, *vpadbox;
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

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(vpadbox), description_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(vpadbox), combo, FALSE, FALSE, 0);
    hpadbox = gtk_hbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(hpadbox), vpadbox, TRUE, TRUE, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE, FALSE, QUESTIONBOX_VPADDING);
    register_setter(combo_setter, GTK_COMBO(combo)->entry, q, obj);

    return DC_OK;
}

static int gtkhandler_select(struct frontend *obj, struct question *q, GtkWidget *qbox)
{

    if (q->prev == NULL && q->next == NULL)
    {
        if ( (strcmp(q->tag, "countrychooser/country-name") == 0) || (strcmp(q->tag, "partman/choose_partition") == 0) )
            return gtkhandler_select_treeview_store(obj, q, qbox);
        else
            return gtkhandler_select_treeview_list(obj, q, qbox);
    }
    else
        return gtkhandler_select_multiple(obj, q, qbox);
}

static int gtkhandler_string(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget *description_box, *entry, *hpadbox, *vpadbox;
    struct frontend_question_data *data;
    const char *defval = question_getvalue(q, "");

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_string() called"); */

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    entry = gtk_entry_new ();
    if (defval != NULL)
        gtk_entry_set_text (GTK_ENTRY(entry), defval);
    else
        gtk_entry_set_text (GTK_ENTRY(entry), "");
    gtk_entry_set_max_length (GTK_ENTRY (entry), STRING_MAX_LENGTH );
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

    g_signal_connect (G_OBJECT(entry), "destroy", G_CALLBACK (free_description_data), data);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(vpadbox), description_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(vpadbox), entry, FALSE, FALSE, 0);
    hpadbox = gtk_hbox_new (FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(hpadbox), vpadbox, TRUE, TRUE, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE, FALSE, QUESTIONBOX_VPADDING);

    register_setter(entry_setter, entry, q, obj);

    if (is_first_question(q))
        gtk_widget_grab_focus(entry);

    return DC_OK;
}

struct question_handlers {
    const char *type;
    gtk_handler *handler;
} question_handlers[] = {
    { "boolean",        gtkhandler_boolean },
    { "multiselect",    gtkhandler_multiselect },
    { "note",           gtkhandler_note },
    { "password",       gtkhandler_password },
    { "select",         gtkhandler_select },
    { "string",         gtkhandler_string },
    { "error",          gtkhandler_note },
    { "text",           gtkhandler_text },
    { "",               NULL },
};

void set_design_elements(struct frontend *obj, GtkWidget *window)
{

    GtkWidget *v_mainbox, *h_mainbox, *logobox, *targetbox, *actionbox, *h_actionbox;
    GtkWidget *button_next, *button_prev, *button_screenshot, *button_cancel;
    GtkWidget *progress_bar, *progress_bar_label, *progress_bar_box, *h_progress_bar_box, *v_progress_bar_box;
    GtkWidget *label_title, *h_title_box, *v_title_box, *logo_button;
    GList *focus_chain = NULL;
    int *ret_val;

    /* A logo is displayed in the upper area of the screen */
    logo_button = gtk_image_new_from_file("/usr/share/graphics/logo_debian.png");
    g_signal_connect_after(G_OBJECT(logo_button), "expose_event", G_CALLBACK(expose_event_callback), obj);
  
    /* A label is used to display the fontend's title */
    label_title = gtk_label_new(NULL);
    ((struct frontend_data*) obj->data)->title = label_title;
    h_title_box = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start(GTK_BOX (h_title_box), label_title, TRUE, TRUE, DEFAULT_PADDING);
    v_title_box = gtk_vbox_new (TRUE, 0);
    gtk_box_pack_start(GTK_BOX (v_title_box), h_title_box, TRUE, TRUE, 0);

    /* This is the box were question(s) will be displayed */
    targetbox = gtk_vbox_new (FALSE, 0);
    ((struct frontend_data*) obj->data)->target_box = targetbox;

    actionbox = gtk_hbutton_box_new();
    h_actionbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX (h_actionbox), actionbox, TRUE, TRUE, DEFAULT_PADDING);
    gtk_button_box_set_layout (GTK_BUTTON_BOX(actionbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing (GTK_BOX(actionbox), DEFAULT_PADDING);

    /* button to take screenshots of the frontend */
    button_screenshot = gtk_button_new_with_label (get_text(obj, "debconf/gtk-button-screenshot", "Screenshot"));
    g_signal_connect (G_OBJECT (button_screenshot), "clicked", G_CALLBACK (screenshot_button_callback), obj );
    gtk_box_pack_start (GTK_BOX(actionbox), button_screenshot, TRUE, TRUE, DEFAULT_PADDING);
    ((struct frontend_data*) obj->data)->button_screenshot = button_screenshot;

    /* Here are the back and forward buttons */
    button_prev = gtk_button_new_with_label (get_text(obj, "debconf/button-goback", "Go Back"));
    ret_val = NEW(int);
    *ret_val = DC_GOBACK;
    gtk_object_set_user_data (GTK_OBJECT(button_prev), ret_val);
    g_signal_connect (G_OBJECT(button_prev), "clicked",
                      G_CALLBACK(exit_button_callback), obj);
    gtk_box_pack_start (GTK_BOX(actionbox), button_prev, TRUE, TRUE, DEFAULT_PADDING);

    button_next = gtk_button_new_with_label (get_text(obj, "debconf/button-continue", "Continue"));
    ret_val = NEW(int);
    *ret_val = DC_OK;
    gtk_object_set_user_data (GTK_OBJECT(button_next), ret_val);
    g_signal_connect (G_OBJECT(button_next), "clicked",
                      G_CALLBACK(exit_button_callback), obj);
    gtk_box_pack_start (GTK_BOX(actionbox), button_next, TRUE, TRUE, DEFAULT_PADDING);
    GTK_WIDGET_SET_FLAGS (button_next, GTK_CAN_DEFAULT);

    ((struct frontend_data*) obj->data)->button_prev = button_prev;
    ((struct frontend_data*) obj->data)->button_next = button_next;
    gtk_widget_set_sensitive (button_prev, FALSE);
    gtk_widget_set_sensitive (button_next, FALSE);

    /* Cancel button is not displayed by default */
    button_cancel = gtk_button_new_with_label (get_text(obj, "debconf/button-cancel", "Cancel"));
    ret_val = NEW(int);
    *ret_val = DC_GOBACK;
    gtk_object_set_user_data (GTK_OBJECT(button_cancel), ret_val);
    g_signal_connect (G_OBJECT(button_cancel), "clicked",
                      G_CALLBACK(cancel_button_callback), obj);
    gtk_box_pack_start (GTK_BOX(actionbox), button_cancel, TRUE, TRUE, DEFAULT_PADDING);
    ((struct frontend_data*) obj->data)->button_cancel = button_cancel;
    gtk_widget_hide(button_cancel);

    /* focus order inside actionbox */
    focus_chain = g_list_append(focus_chain, button_next);
    focus_chain = g_list_append(focus_chain, button_prev);
    gtk_container_set_focus_chain(GTK_CONTAINER(actionbox), focus_chain);
    g_list_free (focus_chain);

    /* Here the the progressbar is placed */
    progress_bar = gtk_progress_bar_new ();
    ((struct frontend_data*)obj->data)->progress_bar = progress_bar;  
    #if GTK_CHECK_VERSION(2,6,0)
    gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR(progress_bar), PANGO_ELLIPSIZE_MIDDLE);
    #endif
    progress_bar_box = gtk_vbox_new (FALSE, 0);
    v_progress_bar_box = gtk_vbox_new (FALSE, 0);
    h_progress_bar_box = gtk_hbox_new (FALSE, 0);
    progress_bar_label = gtk_label_new("");
    ((struct frontend_data*)obj->data)->progress_bar_label = progress_bar_label;
    gtk_misc_set_alignment(GTK_MISC(progress_bar_label), 0, 0);
    gtk_box_pack_start (GTK_BOX(progress_bar_box), progress_bar, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(progress_bar_box), progress_bar_label, FALSE, FALSE, DEFAULT_PADDING);
    gtk_box_pack_start (GTK_BOX(v_progress_bar_box), progress_bar_box, TRUE, TRUE, PROGRESSBAR_VPADDING);
    gtk_box_pack_start (GTK_BOX(h_progress_bar_box), v_progress_bar_box, TRUE, TRUE, PROGRESSBAR_HPADDING);
    ((struct frontend_data*)obj->data)->progress_bar_box = h_progress_bar_box;

    /* Final packaging */
    v_mainbox = gtk_vbox_new (FALSE, 0);
    h_mainbox = gtk_hbox_new (FALSE, 0);
    logobox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start(GTK_BOX (v_mainbox), v_title_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (v_mainbox), h_progress_bar_box, FALSE, FALSE, DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX (v_mainbox), targetbox, TRUE, TRUE, DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX (v_mainbox), h_actionbox, FALSE, FALSE, DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX (h_mainbox), v_mainbox, TRUE, TRUE, DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX (logobox), logo_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (logobox), h_mainbox, TRUE, TRUE, DEFAULT_PADDING);
    gtk_container_add(GTK_CONTAINER(window), logobox);
    
    /* pressing ESC key simulates a user's click on the "Back" button*/
    g_signal_connect_after (window, "key_press_event", G_CALLBACK (key_press_event), obj );
     
}

void *eventhandler_thread()
{
    gdk_threads_enter();
    gtk_main ();
    gdk_threads_leave();
    return 0;
}

static int gtk_initialize(struct frontend *obj, struct configuration *conf)
{
    struct frontend_data *fe_data;
    GtkWidget *window;
	GThread *thread_events_listener;
	GError *err_events_listener = NULL ;
    int args = 1;
    char **name;

    name = malloc(2 * sizeof(char*));
    name[0] = strdup("debconf");
    name[1] = NULL;

    /* INFO(INFO_DEBUG, "GTK_DI - gtk_initialize() called"); */
    obj->data = NEW(struct frontend_data);
    obj->interactive = 1;

    /* It's recomended setting fields in frontend_data structure to NULL,
     * as otherwise older GTKDFB versions may cause segfaults.
     */
    fe_data = obj->data;
    fe_data->window = NULL;
    fe_data->title = NULL;
    fe_data->target_box = NULL;
    fe_data->button_next = NULL;
    fe_data->button_prev = NULL;
    fe_data->button_cancel = NULL;
    fe_data->progress_bar = NULL;
    fe_data->progress_bar_label = NULL;
    fe_data->progress_bar_box = NULL;
    fe_data->setters = NULL;
    fe_data->button_val = DC_NOTOK;

    if( !g_thread_supported() ) {
       g_thread_init(NULL);
       gdk_threads_init();
    }
    else {
        INFO(INFO_DEBUG, "GTK_DI - gtk_initialize() failed to initialize threads\n%s", err_events_listener->message);
		g_error_free ( err_events_listener ) ;
        return DC_NOTOK;
    }
    
    gtk_init (&args, &name);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (window, WINDOW_WIDTH, WINDOW_HEIGHT);
    gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
    set_design_elements (obj, window);
    gtk_rc_reparse_all();
    ((struct frontend_data*) obj->data)->window = window;
    gtk_widget_set_default_direction(get_text_direction(obj));
    gtk_widget_show_all(window);

    button_cond = g_cond_new ();
    button_mutex = g_mutex_new ();
      
	if( (thread_events_listener = g_thread_create((GThreadFunc)eventhandler_thread, NULL, TRUE, &err_events_listener)) == NULL) {
        INFO(INFO_DEBUG, "GTK_DI - gtk_initialize() failed to create events listener thread\n%s", err_events_listener->message);
		g_error_free ( err_events_listener ) ;
        return DC_NOTOK;
	}   

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
    GtkWidget *questionbox, *questionbox_scroll;
    di_slist *plugins;
    int i, j;
    int ret;

    if (q == NULL) return DC_OK;

    data->setters = NULL;

    gdk_threads_enter();

    /* TODO
     * workarund to force dfb to reload keymap at every run, awaiting
     * for dfb to support automatic keymap change detection and reloading
     * (See also bug #381979)
     */

    #if GTK_CHECK_VERSION(2,10,0)
    #ifdef GDK_WINDOWING_DIRECTFB
    dfb_input_device_reload_keymap( dfb_input_device_at( DIDID_KEYBOARD ) );
    #endif
    #else
    dfb_input_device_reload_keymap( dfb_input_device_at( DIDID_KEYBOARD ) );
    #endif

    gtk_rc_reparse_all();

    questionbox = gtk_vbox_new(FALSE, 0);

    /* since all widgets used to display single questions have native
     * scrolling capabilities or do not need scrolling since they're
     * usually small they can manage scrolling be autonomously.
     * vice-versa the most simple approach when displaying multiple questions
     * togheter (whose handling wigets haven't native scrolling capabilities)
     * is to pack them all inside a viewport
     */
    if ( (obj->questions->next==NULL && obj->questions->prev==NULL) )
    {
        gtk_box_pack_start(GTK_BOX(data->target_box), questionbox, TRUE, TRUE, 0);
    }
    else
    {
        questionbox_scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (questionbox_scroll), questionbox);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (questionbox_scroll),
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (questionbox_scroll), GTK_SHADOW_NONE);
        gtk_box_pack_start(GTK_BOX(data->target_box), questionbox_scroll, TRUE, TRUE, DEFAULT_PADDING);
    }

    /* now we can safely handle all other questions, if any */
    j = 0;
    plugins = di_slist_alloc();
    while (q != NULL)
    {
        j++;
        /* INFO(INFO_DEBUG, "GTK_DI - question %d: %s (type %s)", j, q->tag, q->template->type); */
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
                /* we've found the right handler for the question, so we break
                 * the for() loop
                 */
                break;
            }
        }

        if (i == DIM(question_handlers))
            return DC_NOTIMPL; /* TODO: leak? */

        q = q->next;
    }

    if ( obj->methods.can_go_back(obj, q) )
        gtk_widget_set_sensitive (data->button_prev, TRUE);
    else
        gtk_widget_set_sensitive (data->button_prev, FALSE);

    gtk_widget_set_sensitive(GTK_WIDGET(data->button_next), TRUE);
    gtk_widget_set_sensitive (data->button_screenshot, TRUE);

    gtk_button_set_label (GTK_BUTTON(data->button_screenshot), get_text(obj, "debconf/gtk-button-screenshot", "Screenshot") );
    gtk_button_set_label (GTK_BUTTON(data->button_prev), get_text(obj, "debconf/button-goback", "Go Back") );
    gtk_button_set_label (GTK_BUTTON(data->button_next), get_text(obj, "debconf/button-continue", "Continue") );
    gtk_button_set_label (GTK_BUTTON(data->button_cancel), get_text(obj, "debconf/button-cancel", "Cancel") );

    gtk_widget_set_default_direction(get_text_direction(obj));

    /* The "Next" button is activated if the user presses "Enter" */
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET(data->button_next), GTK_CAN_DEFAULT);
    gtk_widget_grab_default (GTK_WIDGET(data->button_next));

    gtk_widget_show_all(data->window);
    gtk_widget_hide(((struct frontend_data*)obj->data)->progress_bar_box) ;
    gtk_widget_hide(((struct frontend_data*)obj->data)->button_cancel) ;

    gdk_threads_leave();

    g_mutex_lock (button_mutex);
    /* frontend blocked here until the user presses either back or forward button */
    g_cond_wait (button_cond, button_mutex);
    g_mutex_unlock (button_mutex);
	
    gdk_threads_enter();

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

    if ( (obj->questions->next==NULL && obj->questions->prev==NULL) )
        gtk_widget_destroy(questionbox);
    else
        gtk_widget_destroy(questionbox_scroll);

    gtk_widget_set_sensitive (data->button_prev, FALSE);
    gtk_widget_set_sensitive (data->button_next, FALSE);

    gdk_threads_leave();

    if (data->button_val == DC_OK)
        return DC_OK;
    else if (data->button_val == DC_GOBACK)
        return DC_GOBACK;
    else
        return DC_OK;
}

static void gtk_set_title(struct frontend *obj, const char *title)
{
    GtkWidget *label_title;
    char *label_title_string;

    /* INFO(INFO_DEBUG, "GTK_DI - gtk_set_title() called"); */

    gdk_threads_enter();
    label_title = ((struct frontend_data*) obj->data)->title;
    gtk_misc_set_alignment(GTK_MISC(label_title), 0, 0);
    label_title_string = malloc(strlen(title) + 10 );
    sprintf(label_title_string,"<b> %s</b>", title);
    gtk_label_set_markup(GTK_LABEL(label_title), label_title_string);
    gdk_threads_leave();
}

static bool gtk_can_go_back(struct frontend *obj, struct question *q)
{
    return (obj->capability & DCF_CAPB_BACKUP);
}

static bool	gtk_can_cancel_progress(struct frontend *obj)
{
    return (obj->capability & DCF_CAPB_PROGRESSCANCEL);
}

static void set_design_elements_while_progressbar_runs(struct frontend *obj)
{
    struct frontend_data *data = (struct frontend_data *) obj->data;

    /* cancel button has to be displayed */
    if (obj->methods.can_cancel_progress(obj)) {
        gtk_widget_hide(data->button_screenshot);
        gtk_widget_hide(data->button_prev);
        gtk_widget_hide(data->button_next);
        gtk_widget_show(data->button_cancel);
        GTK_WIDGET_SET_FLAGS (GTK_WIDGET(data->button_cancel), GTK_CAN_DEFAULT);
        gtk_widget_grab_default (GTK_WIDGET(data->button_cancel));    
    }
    else {
        gtk_widget_set_sensitive (data->button_screenshot, FALSE);
        gtk_widget_set_sensitive (data->button_prev, FALSE);
        gtk_widget_set_sensitive (data->button_next, FALSE);
        gtk_widget_hide(data->button_cancel);
    }

    gtk_widget_show(data->progress_bar_box);
}

static void gtk_progress_start(struct frontend *obj, int min, int max, const char *title)
{
    GtkWidget *progress_bar;

    gdk_threads_enter();
    gtk_rc_reparse_all();
    gtk_button_set_label (GTK_BUTTON(((struct frontend_data*)obj->data)->button_screenshot), get_text(obj, "debconf/gtk-button-screenshot", "Screenshot") );
    gtk_button_set_label (GTK_BUTTON(((struct frontend_data*)obj->data)->button_prev), get_text(obj, "debconf/button-goback", "Go Back") );
    gtk_button_set_label (GTK_BUTTON(((struct frontend_data*)obj->data)->button_next), get_text(obj, "debconf/button-continue", "Continue") );
    gtk_button_set_label (GTK_BUTTON(((struct frontend_data*)obj->data)->button_cancel), get_text(obj, "debconf/button-cancel", "Cancel") );
    gtk_widget_set_default_direction(get_text_direction(obj));
    set_design_elements_while_progressbar_runs(obj);
    DELETE(obj->progress_title);
    obj->progress_title=strdup(title);
    progress_bar = ((struct frontend_data*)obj->data)->progress_bar;
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), obj->progress_title);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0);
    obj->progress_min = min;
    obj->progress_max = max;
    obj->progress_cur = min;
    gdk_threads_leave();

    /* INFO(INFO_DEBUG, "GTK_DI - gtk_progress_start(min=%d, max=%d, title=%s) called", min, max, title); */
}

static int gtk_progress_set(struct frontend *obj, int val)
{
    gdouble progress;
    GtkWidget *progress_bar;
    struct frontend_data *data = (struct frontend_data *) obj->data;
    gdk_threads_enter();
    set_design_elements_while_progressbar_runs(obj);
    
    /* INFO(INFO_DEBUG, "GTK_DI - gtk_progress_set(val=%d) called", val); */

    progress_bar = ((struct frontend_data*)obj->data)->progress_bar;
    gtk_widget_set_sensitive( GTK_WIDGET(progress_bar), TRUE);

    obj->progress_cur = val;
    if ((obj->progress_max - obj->progress_min) > 0)
    {
        progress = (gdouble)(obj->progress_cur - obj->progress_min) /
                   (gdouble)(obj->progress_max - obj->progress_min);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), progress);
    }
    gdk_threads_leave();

    if (data->button_val == DC_OK || data->button_val == DC_GOBACK)
        return (data->button_val);
    else
        return DC_OK;
}

static int gtk_progress_info(struct frontend *obj, const char *info)
{
    GtkWidget *progress_bar_label;
    char *progress_bar_label_string;
    struct frontend_data *data = (struct frontend_data *) obj->data;
    gdk_threads_enter();
    set_design_elements_while_progressbar_runs(obj);
    /* INFO(INFO_DEBUG, "GTK_DI - gtk_progress_info(%s) called", info); */

    progress_bar_label = ((struct frontend_data*)obj->data)->progress_bar_label;
    progress_bar_label_string = malloc(strlen(info) + 10 );
    sprintf(progress_bar_label_string,"<i> %s</i>",info);
    gtk_label_set_markup(GTK_LABEL(progress_bar_label), progress_bar_label_string);
    free(progress_bar_label_string);
    gdk_threads_leave();

    if (data->button_val == DC_OK || data->button_val == DC_GOBACK)
        return (data->button_val);
    else
        return DC_OK;
}

static void gtk_progress_stop(struct frontend *obj)
{
    /* INFO(INFO_DEBUG, "GTK_DI - gtk_progress_stop() called"); */
    gdk_threads_enter();
    gtk_widget_hide( ((struct frontend_data*)obj->data)->progress_bar_box );
    gtk_widget_hide( ((struct frontend_data*)obj->data)->button_cancel );
    gdk_threads_leave();
}

static unsigned long gtk_query_capability(struct frontend *f)
{
    /* INFO(INFO_DEBUG, "GTK_DI - gtk_query_capability() called"); */
    return DCF_CAPB_BACKUP;
}

struct frontend_module debconf_frontend_module =
{
    initialize: gtk_initialize,
    go: gtk_go,
    set_title: gtk_set_title,
    can_go_back: gtk_can_go_back,
    can_cancel_progress: gtk_can_cancel_progress,
    progress_start: gtk_progress_start,
    progress_info: gtk_progress_info,
    progress_set: gtk_progress_set,
    progress_stop: gtk_progress_stop,
    query_capability: gtk_query_capability,
};
