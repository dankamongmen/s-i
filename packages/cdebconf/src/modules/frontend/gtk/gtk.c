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

#include <directfb.h>

/* for dfb_input_device_reload_keymap() and dfb_input_device_at() */
#include <core/input.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

/* maximum length for string questions */
#define STRING_MAX_LENGTH 128

/* used to define horizontal and vertical padding of progressbar */
#define PROGRESSBAR_HPADDING 60
#define PROGRESSBAR_VPADDING 60

/* paths to images */
#define NOTE_IMAGE_PATH "/usr/share/graphics/note_icon.png"
#define WARNING_IMAGE_PATH "/usr/share/graphics/warning_icon.png"
#define LOGO_IMAGE_PATH "/usr/share/graphics/logo_debian.png"

/* Make sure this is called in a GDK thread-safe way
 */
void update_frontend_title(struct frontend * obj, char * title);

char * progressbar_title = NULL;

typedef int (gtk_handler)(struct frontend * obj, struct question * q,
                          GtkWidget * questionbox);

static GCond * button_cond = NULL;
static GMutex * button_mutex = NULL;

/* A struct to let a question handler store appropriate set functions that will be called after
   gtk_main has quit */
struct setter_struct {
    void (*func)(void *, struct question *);
    void * data;
    struct question * q;
    struct setter_struct * next;
};

typedef int (custom_func_t)(struct frontend *, struct question *, GtkWidget *);

static char const * get_text(struct frontend * obj, char const * template,
                             char const * fallback);

void register_setter(void (*func)(void *, struct question *),
                     void * data, struct question * q,
                     struct frontend * obj)
{
    struct setter_struct *s;
    struct frontend_data *fe_data = obj->data;

    s = malloc(sizeof (struct setter_struct));
    s->func = func;
    s->data = data;
    s->q = q;
    s->next = fe_data->setters;
    fe_data->setters = s;
}

void free_description_data(GtkObject * obj,
                           struct frontend_question_data * data)
{
    free(data);
}

gboolean is_first_question(struct question * q)
{
    struct question * crawl;

    crawl = q;

    while (NULL != crawl->prev) {
        if (0 != strcmp(crawl->prev->template->type, "note")) {
            return FALSE;
        }
        crawl = crawl->prev;
    }
    return TRUE;
}

static void bool_setter(void * check, struct question * q)
{
    gboolean check_value;

    check_value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
    question_setvalue(q, check_value ? "true" : "false");
}

static void entry_setter(void *entry, struct question *q)
{
    question_setvalue(q, gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void combo_setter(void * entry, struct question * q)
{
    char ** choices;
    char ** choices_translated;
    int i;
    int count;
    int * tindex = NULL;
    gchar const * indices;
    
    indices = q_get_indices(q);

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0) {
        return /* DC_NOTOK */;
    }
    choices = malloc(sizeof (char *) * count);
    choices_translated = malloc(sizeof (char *) * count);
    tindex = malloc(sizeof (int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q),
                           indices, choices, choices_translated,
                           tindex, count) != count) {
        return /* DC_NOTOK */;
    }

    for (i = 0; i < count; i++) {
        if (0 == strcmp(gtk_entry_get_text(GTK_ENTRY(entry)),
                    choices_translated[i])) {
            question_setvalue(q, choices[tindex[i]]);
        }

        free(choices[tindex[i]]);
        free(choices_translated[i]);
    }
    free(choices);
    free(choices_translated);
    free(tindex);
}

static void select_setter (void * treeview, struct question * q)
{
    GtkTreeSelection * selection;
    GtkTreeModel * model;
    GtkTreeIter iter;
    int i;
    int count;
    int *tindex = NULL;
    char ** choices;
    char ** choices_translated;
    gchar const * indices;
    gchar * name;
    
    indices = q_get_indices(q);

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0) {
        return;
    }
    choices = malloc(sizeof (char *) * count);
    choices_translated = malloc(sizeof (char *) * count);
    tindex = malloc(sizeof (int) * count);

    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices,
                           choices, choices_translated, tindex,
                           count) != count) {
        return;
    }

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (treeview));
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, SELECT_COL_NAME, &name, -1);
        //g_print ("selected row is: %s\n", name);
        for (i = 0; i < count; i++) {
            if (strcmp(name, choices_translated[i]) == 0) {
                question_setvalue(q, choices[tindex[i]]);
            }
            free(choices[tindex[i]]);
            free(choices_translated[i]);
        }
        g_free(name);
    }

    free(choices);
    free(choices_translated);
    free(tindex);
}

static void multiselect_single_setter(void * treeview, struct question * q)
{
    int i;
    int count;
    char ** choices;
    char ** choices_translated;
    int * tindex = NULL;
    gchar * indices;
    gchar * result = NULL;
    gchar * copy = NULL;
    GtkTreeModel * model;
    GtkTreePath * path;
    GtkTreeIter iter;
    gboolean bool_var;

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0) {
        return /* DC_NOTOK */;
    }
    choices = malloc(sizeof (char *) * count);
    choices_translated = malloc(sizeof (char *) * count);
    tindex = malloc(sizeof (int) * count);
    indices = q_get_indices(q);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices,
                           choices, choices_translated, tindex,
                           count) != count) {
        return /* DC_NOTOK */;
    }

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    for (i = 0; i < count; i++) {
        path = gtk_tree_path_new_from_indices(i, -1);
        gtk_tree_model_get_iter(model, &iter, path);
        gtk_tree_model_get(model, &iter, MULTISELECT_COL_BOOL, &bool_var, -1);

        if (NULL != result && 1 == bool_var) {
            printf("Option %d active\n", i);
            copy = g_strdup(result);
            free(result);
            result = g_strconcat(copy, ", ", choices[tindex[i]], NULL);
            free(copy);
        } else if (NULL == result && 1 == bool_var) {
            result = g_strdup(choices[tindex[i]]);
        }

        gtk_tree_path_free(path);
    }

    if (NULL == result) {
        result = g_strdup("");
    }

    question_setvalue(q, result);
    free(result);
    free(choices);
    free(choices_translated);
    free(tindex);
    free(indices);
}

static void multiselect_multiple_setter(void * check_container,
                                        struct question * q)
{
    gchar * result = NULL;
    gchar * copy = NULL;
    GList * check_list;
    int i;
    int count;
    char ** choices, ** choices_translated;
    int * tindex = NULL;
    gchar const * indices;
    
    indices = q_get_indices(q);

    check_list = gtk_container_get_children(GTK_CONTAINER(check_container));
    while (NULL != check_list) {
        if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(check_list->data))) {
            count = strgetargc(q_get_choices_vals(q));
            if (count <= 0) {
                return /* DC_NOTOK */;
            }
            choices = malloc(sizeof (char *) * count);
            choices_translated = malloc(sizeof (char *) * count);
            tindex = malloc(sizeof (int) * count);
            if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q),
                                   indices, choices, choices_translated,
                                   tindex, count) != count) {
                return /* DC_NOTOK */;
            }

            for (i = 0; i < count; i++) {
                if (0 == strcmp(gtk_button_get_label(
                                    GTK_BUTTON(check_list->data)),
                                    choices_translated[i])) {
                    if (result != NULL) {
                        copy = g_strdup(result);
                        free(result);
                        result = g_strconcat(copy, ", ", choices[tindex[i]],
                                             NULL /* EOL */);
                        free(copy);
                    } else {
                        result = g_strdup(choices[tindex[i]]);
                    }
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
    if (!result) {
        result = g_strdup("");
    }
    question_setvalue(q, result);
    g_list_free(check_list);
    free(result);
}

static void call_setters(struct frontend * obj)
{
    struct frontend_data * data = (struct frontend_data *) obj->data;
    struct setter_struct * s;
    struct setter_struct * p;

    /* INFO(INFO_DEBUG, "GTK_DI - call_setters() called"); */

    s = data->setters;
    while (NULL != s) {
        (*s->func)(s->data, s->q);
        p = s;
        s = s->next;
        free(p);
    }
}

static gboolean expose_event_callback(GtkWidget * wid,
                                      GdkEventExpose * event,
                                      struct frontend * obj)
{
    PangoLayout * layout; 
    gint w;
    gint h;
    gchar * message;
    char * text;

    if (NULL != obj->info) {
        text = q_get_description(obj->info);
        if (NULL != text) {
            message = malloc(strlen(text) + 42);
            sprintf(message,"<b><span foreground=\"#ffffff\">%s</span></b>",
                    text);
            layout = gtk_widget_create_pango_layout(
                wid, NULL /* no text for the layout */);
            pango_layout_set_markup(layout, message, strlen(message));
            /* XXX: free pango_font_description */
            pango_layout_set_font_description(layout,
                pango_font_description_from_string("Sans 12"));
            pango_layout_get_pixel_size(layout, &w, &h);
            /* obj->info is drawn over the debian banner, top-right corner of
             * the screen */
            /* XXX: damn magic numbers */
            gdk_draw_layout(wid->window, gdk_gc_new(wid->window),
                            WINDOW_WIDTH - w - 4 - DEFAULT_PADDING * 2,
                            4, layout);
            g_free(message);
            g_free(text);
        }
        free(text);
    }
    return FALSE;
}


/* Scrolling to default row in SELECT questions has to be done after the
 * treeview has been realized
 */
static void treeview_exposed_callback(
    GtkWidget * widget, GdkEventExpose * event,
    struct treeview_expose_callback_data * data)
{
    GtkTreePath * path;
    
    /* XXX: can be NULL */
    path = gtk_tree_path_new_from_string(data->path);
    /* XXX: magic numbers */
    gtk_tree_view_scroll_to_cell(
        GTK_TREE_VIEW(widget), path,
        NULL /* don't scroll to a particular column */,
        TRUE /* please align */,
        0.5 /* center row */,
        0 /* left column */);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), path,
                             NULL /* don't focus a particular column */,
                             FALSE /* don't start editing */);
    gtk_tree_path_free(path);
    g_signal_handler_disconnect(G_OBJECT(widget), data->callback_function);
    free(data);
}

static void screenshot_button_callback(GtkWidget * button,
                                       struct frontend * obj)
{
    GdkWindow * gdk_window;
    GdkPixbuf * gdk_pixbuf;
    GtkWidget * window;
    GtkWidget * frame;
    GtkWidget * message_label;
    GtkWidget * title_label;
    GtkWidget * h_box;
    GtkWidget * v_box;
    GtkWidget * v_box_outer;
    GtkWidget * close_button;
    GtkWidget * actionbox;
    GtkWidget * separator;
    gint x;
    gint y;
    gint width;
    gint height;
    gint depth;
    int i;
    size_t j;
    char screenshot_name[256];
    char popup_message[256];
    char * tmp;

    /* XXX: can be NULL? */
    gdk_window = gtk_widget_get_parent_window(button);
    /* XXX: can fail? */
    gdk_window_get_geometry(gdk_window, &x, &y, &width, &height, &depth);
    /* XXX: magic numbers */
    gdk_pixbuf = gdk_pixbuf_get_from_drawable(
        NULL, gdk_window, gdk_colormap_get_system(), 0, 0, 0, 0,
        width, height);
    i = 0;
    while (TRUE) {
        sprintf(screenshot_name, "%s_%d.png", obj->questions->tag, i);
        for(j = 0; j < strlen(screenshot_name); j++) {
            if ('/' == screenshot_name[j]) {
                screenshot_name[j] = '_';
            }
        }
        sprintf(popup_message, "/var/log/%s", screenshot_name);
        sprintf(screenshot_name, "%s", popup_message);
        if (!access(screenshot_name, R_OK)) {
            i++;
        } else {
            /* printf ("name: %s.png\nx: %d\ny: %d\nwidth: %d\nheight: %d\ndepth=%d\n", screenshot_name, x, y, width, height, depth); */
            break;
        }
    }
    /* XXX */
    gdk_pixbuf_save(gdk_pixbuf, screenshot_name, "png", NULL, NULL);
    g_object_unref(gdk_pixbuf);

    /* A message inside a popup window tells the user the screenshot has
     * been saved correctly
     */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE /* not resizable */);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE /* no decoration */);
    gtk_container_set_border_width(GTK_CONTAINER(window), 0 /* no border */);

    title_label = gtk_label_new(
        get_text(obj, "debconf/gtk-button-screenshot", "Screenshot"));
    gtk_misc_set_alignment(GTK_MISC(title_label),
                           0 /* left aligned */,
                           0 /* top aligned */);

    /* XXX: rewrite the next two lines */
    tmp = malloc(strlen(get_text(obj, "debconf/gtk-button-screenshot",
                 "Screenshot")) + 8);
    sprintf(tmp,"<b>%s</b>",
            get_text(obj, "debconf/gtk-button-screenshot", "Screenshot"));
    gtk_label_set_markup(GTK_LABEL(title_label), tmp);

    sprintf(popup_message,
            get_text(obj, "debconf/gtk-screenshot-saved",
            "Screenshot saved as %s"), screenshot_name);
    message_label = gtk_label_new(popup_message);

    actionbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(actionbox), GTK_BUTTONBOX_END);
    close_button = gtk_button_new_with_label(
        get_text(obj, "debconf/button-continue", "Continue"));
    g_signal_connect_swapped(
        G_OBJECT(close_button), "clicked",
        G_CALLBACK(gtk_widget_destroy),
        window);
    gtk_box_pack_end(GTK_BOX(actionbox), close_button,
                     TRUE /* expand */, TRUE /* fill */, DEFAULT_PADDING);

    v_box = gtk_vbox_new(FALSE /* don't make children equal */,
                         DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(v_box), title_label, FALSE /* don't expand */,
                       FALSE /* don't fill */, 0 /* no padding */);
    gtk_box_pack_start(GTK_BOX(v_box), message_label, FALSE /* don't expand */,
                       FALSE /* don't fill */, DEFAULT_PADDING);

    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(v_box), separator, FALSE /* don't expand */,
                       FALSE /* don't fill */, 0 /* no padding */);
    gtk_box_pack_start(GTK_BOX(v_box), actionbox, FALSE /* don't expand */,
                       FALSE /* don't fill */, 0 /* no padding */);

    h_box = gtk_hbox_new(FALSE /* don't make children equal */,
                         DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(h_box), v_box, FALSE /* don't expand */,
                       FALSE /* don't fill */, DEFAULT_PADDING);

    v_box_outer = gtk_vbox_new(FALSE /* don't make children equal */,
                               DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(v_box_outer), h_box, FALSE /* don't expand */,
                       FALSE /* don't fill */, DEFAULT_PADDING);

    frame = gtk_frame_new(NULL /* no label */);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
    gtk_container_add(GTK_CONTAINER(frame), v_box_outer);
    gtk_container_add(GTK_CONTAINER(window), frame);
    gtk_widget_show_all(window);

    free(tmp);
}

static void multiselect_single_callback(GtkCellRendererToggle * cell,
                                        gchar const * path_string,
                                        struct question_treemodel_data * data)
{
    GtkTreeModel * model;
    GtkTreePath * path;
    GtkTreeIter iter;
    gboolean bool_var;

    model = (GtkTreeModel *) data->treemodel;
    path = gtk_tree_path_new_from_string(path_string);
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, MULTISELECT_COL_BOOL, &bool_var, -1);
    bool_var ^= 1;
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, MULTISELECT_COL_BOOL,
                       bool_var, -1);
    gtk_tree_path_free(path);
}

static gboolean key_press_event(GtkWidget * widget, GdkEvent * event,
                                struct frontend * obj)
{
    GdkEventKey * key = (GdkEventKey *) event;
    struct frontend_data * data = (struct frontend_data *) obj->data;
    struct question * q = obj->questions;
    
    if (GDK_Escape == key->keyval && obj->methods.can_go_back(obj, q)) {
        /* INFO(INFO_DEBUG, "GTK_DI - ESC key pressed\n"); */
        gtk_button_clicked(GTK_BUTTON(data->button_prev));
    }

    return TRUE;
}

static void exit_button_callback(GtkWidget * button, struct frontend * obj)
{
    int value;
    void * ret;
    struct frontend_data * data = (struct frontend_data *) obj->data;

    ret = gtk_object_get_user_data(GTK_OBJECT(button));
    value = *(int *) ret;

    /* INFO(INFO_DEBUG,
     *      "GTK_DI - exit_button_callback() called, value: %d", value); */

    data->button_val = value;

    g_mutex_lock(button_mutex);
    /* gtk_go() gets unblocked */ 
    g_cond_signal(button_cond);
    g_mutex_unlock(button_mutex);
}

static void cancel_button_callback(GtkWidget * button, struct frontend * obj)
{
    struct frontend_data * data = (struct frontend_data *) obj->data;

    data->button_val = DC_GOBACK;
}

/* catches double-clicks, SPACEBAR, ENTER keys pressure for SELECT questions */
static void select_onRowActivated(GtkTreeView * treeview,
                                  GtkTreePath * path,
                                  GtkTreeViewColumn * col,
                                  struct frontend_data * data)
{
    gtk_button_clicked(GTK_BUTTON(data->button_next));
}

/* XXX: proofread every call to get_text -> they should free the result string
 */
static const char * get_text(struct frontend * obj,
                             char const * template,
                             char const * fallback )
{
    struct question * q = obj->qdb->methods.get(obj->qdb, template);

    return q ? q_get_description(q) : fallback;
}

static GtkTextDirection get_text_direction(struct frontend * obj)
{
    char const * dirstr;
    
    dirstr = get_text(
        obj, "debconf/text-direction", "LTR - default text direction");

    if ('R' == dirstr[0]) {
        return GTK_TEXT_DIR_RTL;
    }
    return GTK_TEXT_DIR_LTR;
}

static GtkWidget * display_descriptions(struct question * q,
                                        struct frontend * obj)
{
    struct frontend_data * data = (struct frontend_data *) obj->data;
    GtkWidget * description_view;
    GtkWidget * ext_description_view = NULL;
    GtkWidget * returned_box;
    GtkWidget * description_box;
    GtkWidget * icon_box;
    GtkWidget * icon_button;
    GtkTextBuffer * description_buffer;
    GtkTextBuffer * ext_description_buffer;
    GdkColor * bg_color;
    GtkTextIter start;
    GtkTextIter end;
    GtkStyle * style;

    style = gtk_widget_get_style(data->window);
    bg_color = style->bg;

    description_box = gtk_vbox_new(FALSE /* don't make children equal */,
                                   0 /* no spacing */);
    icon_box = gtk_vbox_new(FALSE /* don't make children equal */,
                            0 /* no spacing */);
    returned_box = gtk_hbox_new(FALSE /* don't make children equal */,
                                0 /* no spacing */);

    /* here is created the question's extended description, but only
     * if the question's extended description actually exists
     */
    if (strlen(q_get_extended_description(q)) > 0) {
        ext_description_view = gtk_text_view_new();
        ext_description_buffer = gtk_text_view_get_buffer(
            GTK_TEXT_VIEW(ext_description_view));
        gtk_text_buffer_set_text(ext_description_buffer,
            q_get_extended_description(q), -1);
        gtk_text_view_set_editable(
            GTK_TEXT_VIEW(ext_description_view), FALSE /* not editable */);
        gtk_text_view_set_cursor_visible(
            GTK_TEXT_VIEW(ext_description_view), FALSE /* not editable */);
        gtk_text_view_set_wrap_mode(
            GTK_TEXT_VIEW(ext_description_view), GTK_WRAP_WORD);
        gtk_widget_modify_base(GTK_WIDGET(ext_description_view),
                               GTK_STATE_NORMAL, bg_color);
    }

    /* here is created the question's description */
    description_view = gtk_text_view_new();
    description_buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(description_view));
    gtk_text_buffer_set_text(description_buffer, q_get_description(q),
                             -1 /* until '\0' */);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(description_view),
                               FALSE /* not editable */);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(description_view),
                                     FALSE /* not visible */);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(description_view), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(description_view), 4);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(description_view), 4);
    gtk_text_buffer_create_tag(description_buffer, "italic", "style",
                               PANGO_STYLE_ITALIC, NULL);
    /* XXX: is that needed? */
    g_object_set_data(G_OBJECT(description_view), "tag", (void *) "italic");
    gtk_text_buffer_get_start_iter(description_buffer, &start);
    gtk_text_buffer_get_end_iter(description_buffer, &end);
    gtk_text_buffer_apply_tag_by_name(description_buffer, "italic",
                                      &start, &end);
    gtk_widget_modify_base(GTK_WIDGET(description_view),
                           GTK_STATE_NORMAL, bg_color);

    gtk_container_set_focus_chain(GTK_CONTAINER(description_box),
                                  NULL /* empty list */);

    if (0 == strcmp(q->template->type, "note") ||
        0 == strcmp(q->template->type, "error")) {
        gtk_box_pack_start(GTK_BOX(description_box), description_view,
                           FALSE /* don't expand */,
                           FALSE /* don't fill */,
                           3 /* padding */);
        if (strlen(q_get_extended_description(q)) > 0) {
            gtk_box_pack_start(GTK_BOX(description_box),
                               ext_description_view, FALSE /* don't expand */,
                               FALSE /* don't fill */, 2 /* padding */);
        }
    } else {
        if (strlen(q_get_extended_description(q)) > 0) {
            gtk_box_pack_start(GTK_BOX(description_box), ext_description_view,
                               FALSE /* don't expand */,
                               FALSE /* don't fill */, 2 /* padding */);
        }
        gtk_box_pack_start(GTK_BOX(description_box), description_view,
                           FALSE /* don't expand */, FALSE /* don't fill */,
                           3 /* padding */);
    }

    if (0 == strcmp(q->template->type, "note")) {
        icon_button = gtk_image_new_from_file(NOTE_IMAGE_PATH);
        gtk_box_pack_start(GTK_BOX(icon_box), icon_button,
                           FALSE /* don't expand */, FALSE /* don't fill */,
                           3 /* padding */);
        gtk_box_pack_start(GTK_BOX(returned_box), icon_box,
                           FALSE /* don't expand */, FALSE /* don't fill */,
                           3 /* padding */);
    } else if (0 == strcmp(q->template->type, "error")) {
        icon_button = gtk_image_new_from_file(WARNING_IMAGE_PATH);
        gtk_box_pack_start(GTK_BOX(icon_box), icon_button,
                           FALSE /* don't expand */, FALSE /* don't fill */,
                           3 /* padding */);
        gtk_box_pack_start(GTK_BOX(returned_box), icon_box,
                           FALSE /* don't expand */, FALSE /* don't fill */,
                           3 /* padding */);
    }

    gtk_box_pack_start(GTK_BOX(returned_box), description_box,
                       TRUE /* expand */, TRUE /* fill */, 3 /* padding */);

    return returned_box;
}

static int gtkhandler_boolean(struct frontend * obj, struct question * q,
                              GtkWidget * qbox)
{
    GtkWidget * description_box;
    GtkWidget * radio_false;
    GtkWidget * radio_true;
    GtkWidget * hpadbox;
    GtkWidget * vpadbox;
    struct frontend_question_data * data;
    char const * defval = question_getvalue(q, "");

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_boolean() called"); */

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    radio_false = gtk_radio_button_new_with_label(NULL /* new group */,
        question_get_text(obj, "debconf/no", "No"));
    radio_true = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(radio_false),
        question_get_text(obj, "debconf/yes", "Yes"));

    if (0 == strcmp(defval, "true")) {
        /* XXX: only one needed? */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_false), FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_true), TRUE);
    } else {
        /* XXX: only one needed? */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_false), TRUE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_true), FALSE);
    }

    g_signal_connect(G_OBJECT(radio_true), "destroy",
                     G_CALLBACK(free_description_data), data);
    description_box = display_descriptions(q, obj);
    vpadbox = gtk_vbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(vpadbox), description_box,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* padding */);
    gtk_box_pack_start(GTK_BOX(vpadbox), radio_false,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* padding */);
    gtk_box_pack_start(GTK_BOX(vpadbox), radio_true,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* padding */);
    hpadbox = gtk_hbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX (hpadbox), vpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE /* don't expand */,
                       FALSE /* don't fill */, QUESTIONBOX_VPADDING);
    if (is_first_question(q)) {
        if (0 == strcmp(defval, "true")) {
            gtk_widget_grab_focus(radio_true);
        } else {
            gtk_widget_grab_focus(radio_false);
        }
    }

    register_setter(bool_setter, radio_true, q, obj);

    return DC_OK;
}

static int gtkhandler_multiselect_single(struct frontend * obj, 
                                         struct question * q,
                                         GtkWidget * qbox)
{
    GtkWidget * description_box;
    GtkWidget * hpadbox;
    GtkWidget * vpadbox;
    char ** choices;
    char ** choices_translated;
    char ** defvals;
    int i;
    int j;
    int count;
    int defcount;
    int flag_default_found;
    struct question_treemodel_data * data;
    int * tindex = NULL;
    gchar const * indices;
    GtkTreeModel * model;
    GtkListStore * store;
    GtkTreeIter iter;
    GtkWidget * view;
    GtkWidget * scroll;
    GtkWidget * frame;
    GtkCellRenderer * renderer;
    GtkCellRenderer * renderer_check;
    GtkTreePath * path;

    indices = q_get_indices(q);

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0) {
        return DC_NOTOK;
    }

    choices = malloc(sizeof (char *) * count);
    choices_translated = malloc(sizeof (char *) * count);
    tindex = malloc(sizeof (int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q),
                           indices, choices, choices_translated, tindex,
                           count) != count) {
        return DC_NOTOK;
    }

    defvals = malloc(sizeof (char *) * count);

    defcount = strchoicesplit(question_getvalue(q, ""), defvals, count);

    if (defcount < 0) {
        return DC_NOTOK;
    }

    view = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
    store = gtk_list_store_new(MULTISELECT_NUM_COLS, G_TYPE_INT,
                               G_TYPE_STRING);

    renderer_check = gtk_cell_renderer_toggle_new();
    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(view), -1, NULL, renderer_check, "active",
        MULTISELECT_COL_BOOL, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(view), -1 /* insert at the end */,
        NULL /* no title */, renderer,
        /* column: */ "text", MULTISELECT_COL_NAME,
        NULL /* end of list */);

    model = GTK_TREE_MODEL(store);
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
    data = NEW(struct question_treemodel_data);
    data->q = q;
    data->treemodel = model;
    g_signal_connect(G_OBJECT(renderer_check), "toggled",
        G_CALLBACK(multiselect_single_callback), data);
    g_signal_connect(G_OBJECT(view), "destroy",
        G_CALLBACK(free_description_data), data);
    g_object_unref(model);

    for (i = 0; i < count; i++) {
        flag_default_found = FALSE;
        for (j = 0; j < defcount; j++) {
            if (0 == strcmp(choices[tindex[i]], defvals[j])) {
                gtk_list_store_insert_with_values(
                    store, &iter, i /* position */,
                    /* column: */ MULTISELECT_COL_BOOL, TRUE,
                    /* column: */ MULTISELECT_COL_NAME, choices_translated[i],
                    -1 /* end of list */);
                flag_default_found = TRUE;
                break;
            }
        }

        if (FALSE == flag_default_found) {
            gtk_list_store_insert_with_values(
                store, &iter, i /* position */,
                /* column: */ MULTISELECT_COL_BOOL, FALSE,
                /* column: */ MULTISELECT_COL_NAME, choices_translated[i],
                -1 /* end of list */);
        }

        free(choices[tindex[i]]);
        free(choices_translated[i]);
    }
        
    /* by default the first row gets selected if no default option is specified
     * */
    gtk_tree_model_get_iter_first(model,&iter);
    path = gtk_tree_model_get_path(model, &iter);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(view), path,
                             MULTISELECT_COL_BOOL,
                             FALSE /* do not start editing */);
    gtk_tree_path_free(path);

    free(choices);
    free(choices_translated);
    free(tindex);
    for (j = 0; j < defcount; j++) {
        free(defvals[j]);
    }
    free(defvals);

    scroll = gtk_scrolled_window_new(NULL /* create horizontal adjustement */,
                                     NULL /* create vertical adjustement */);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    description_box = display_descriptions(q, obj);
    vpadbox = gtk_vbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(vpadbox), description_box,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* padding */);
    frame = gtk_frame_new(NULL /* no label */);
    gtk_container_add(GTK_CONTAINER(frame), scroll);
    gtk_box_pack_start(GTK_BOX(vpadbox), frame, TRUE /* expand */,
                       TRUE /* fill */, 0 /* padding */);
    hpadbox = gtk_hbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(hpadbox), vpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_VPADDING);
    gtk_widget_grab_focus(view);

    register_setter(multiselect_single_setter, view, q, obj);

    return DC_OK;
}

static int gtkhandler_multiselect_multiple(struct frontend * obj,
                                           struct question * q,
                                           GtkWidget * qbox)
{
    GtkWidget * description_box;
    GtkWidget * check_container;
    GtkWidget * check;
    GtkWidget * hpadbox;
    GtkWidget  *vpadbox;
    char ** choices;
    char ** choices_translated;
    char ** defvals;
    int i;
    int j;
    int count;
    int defcount;
    struct frontend_question_data * data;
    int * tindex = NULL;
    gchar const * indices;
    
    indices = q_get_indices(q);
    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0) {
        return DC_NOTOK;
    }

    choices = malloc(sizeof (char *) * count);
    choices_translated = malloc(sizeof (char *) * count);
    tindex = malloc(sizeof (int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices,
                           choices, choices_translated, tindex,
                           count) != count) {
        return DC_NOTOK;
    }

    defvals = malloc(sizeof (char *) * count);

    defcount = strchoicesplit(question_getvalue(q, ""), defvals, count);
    if (defcount < 0) {
        return DC_NOTOK;
    }
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

    check_container = gtk_vbox_new(FALSE /* don't make children equal */,
                                   0 /* padding */);

    g_signal_connect(G_OBJECT(check_container), "destroy",
                     G_CALLBACK(free_description_data), data);

    for (i = 0; i < count; i++) {
        check = gtk_check_button_new_with_label(choices_translated[i]);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                     FALSE /* deactivate */);
        for (j = 0; j < defcount; j++) {
            if (0 == strcmp(choices[tindex[i]], defvals[j])) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                             TRUE /* activate */);
            }
        }
        gtk_box_pack_start(GTK_BOX(check_container), check,
                           FALSE /* don't expand */, FALSE /* don't fill */,
                           0 /* padding */);
        if (is_first_question(q) && 0 == i) {
            gtk_widget_grab_focus(check);
        }

        free(choices[tindex[i]]);
        free(choices_translated[i]);
    }

    free(choices);
    free(choices_translated);
    free(tindex);
    for (j = 0; j < defcount; j++) {
        free(defvals[j]);
    }
    free(defvals);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(vpadbox), description_box, TRUE /* expand */,
                       TRUE /* fill */, 0 /* padding */);
    gtk_box_pack_start(GTK_BOX(vpadbox), check_container, TRUE /* expand */,
                       TRUE /* fill */, 0 /* padding */);
    hpadbox = gtk_hbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(hpadbox), vpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE /* don't expand */,
                       FALSE /* don't fill */, QUESTIONBOX_VPADDING);

    register_setter(multiselect_multiple_setter, check_container, q, obj);

    return DC_OK;
}


static int gtkhandler_multiselect(struct frontend * obj,
                                  struct question * q, GtkWidget * qbox)
{
    if (NULL == q->prev && NULL == q->next) {
        return gtkhandler_multiselect_single(obj, q, qbox);
    } else {
        return gtkhandler_multiselect_multiple(obj, q, qbox);
    }
}

static int gtkhandler_note(struct frontend * obj,
                           struct question * q,
                           GtkWidget * qbox)
{
    GtkWidget * hpadbox;
    GtkWidget * vpadbox;
    GtkWidget * description_box;

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_note() called"); */

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(vpadbox), description_box,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* padding */);
    hpadbox = gtk_hbox_new(FALSE /* don't make children equal */, 
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(hpadbox), vpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE /* don't expand */,
                       FALSE /* don't fill */, QUESTIONBOX_VPADDING);

    return DC_OK;
}

static int gtkhandler_text(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    return gtkhandler_note(obj, q, qbox);
}

static int gtkhandler_password(struct frontend *obj, struct question *q, GtkWidget *qbox)
{
    GtkWidget * description_box;
    GtkWidget * entry;
    GtkWidget * hpadbox;
    GtkWidget * vpadbox;
    struct frontend_question_data * data;

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_password() called"); */

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), STRING_MAX_LENGTH);
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE /* invisible */);
    gtk_entry_set_activates_default(GTK_ENTRY(entry),
                                    TRUE /* activate on Enter */);

    g_signal_connect(G_OBJECT(entry), "destroy",
                     G_CALLBACK(free_description_data), data);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(vpadbox), description_box,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* expand */);
    gtk_box_pack_start(GTK_BOX(vpadbox), entry, FALSE /* don't expand */,
                       FALSE /* don't fill */, 0 /* padding */);
    hpadbox = gtk_hbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(hpadbox), vpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE /* don't expand */,
                       FALSE /* don't fill */, QUESTIONBOX_VPADDING);
    if (is_first_question(q)) {
        gtk_widget_grab_focus(entry);
    }

    register_setter(entry_setter, entry, q, obj);

    return DC_OK;
}

static int gtkhandler_select_single_list(struct frontend * obj,
                                         struct question * q,
                                         GtkWidget * qbox)
{
    char ** choices;
    char ** choices_translated;
    int i;
    int count;
    struct frontend_question_data * data;
    char const * defval;
    int * tindex = NULL;
    gchar const * indices;
    GtkWidget * hpadbox;
    GtkWidget * vpadbox;
    GtkWidget * description_box;
    int flag_default_set = FALSE;
    GtkTreeModel * model;
    GtkListStore * store;
    GtkTreeIter iter;
    GtkWidget * view;
    GtkWidget * scroll;
    GtkWidget * frame;
    GtkCellRenderer * renderer;
    GtkTreeSelection * selection;
    struct treeview_expose_callback_data * expose_data; 

    defval = question_getvalue(q, "");
    indices = q_get_indices(q);
    expose_data = NEW(struct treeview_expose_callback_data);

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_select_single_list() called"); */

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0) {
        return DC_NOTOK;
    }
    choices = malloc(sizeof (char *) * count);
    choices_translated = malloc(sizeof (char *) * count);
    tindex = malloc(sizeof (int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices,
                           choices, choices_translated, tindex,
                           count) != count) {
        return DC_NOTOK;
    }

    view = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view),
                                      FALSE /* invisible headers */);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(view), -1 /* insert at the end */,
        q_get_description(q) /* title */, renderer,
        "text", SELECT_COL_NAME,
        NULL /* end of list */);
    store = gtk_list_store_new(SELECT_NUM_COLS, G_TYPE_STRING, G_TYPE_UINT);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    g_signal_connect(G_OBJECT(view), "row-activated",
                     G_CALLBACK(select_onRowActivated),
                     obj->data);
    g_signal_connect(G_OBJECT(view), "destroy",
                     G_CALLBACK(free_description_data), data);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view),
                                    TRUE /* enable typeahead */);
    gtk_tree_selection_set_mode(
        gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
        GTK_SELECTION_BROWSE);
    model = GTK_TREE_MODEL(store);
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

    for (i = 0; i < count; i++) {
        gtk_list_store_insert_with_values(
            store, &iter, i /* position */,
            /* column: */ SELECT_COL_NAME, choices_translated[i],
            -1 /* end of list */);
        if (!flag_default_set && NULL != defval &&
            0 == strcmp(choices[tindex[i]], defval)) {
            expose_data->path = gtk_tree_path_to_string(
                gtk_tree_model_get_path(model, &iter));
            expose_data->callback_function =
                g_signal_connect_after(G_OBJECT(view), "expose_event",
                                       G_CALLBACK(treeview_exposed_callback),
                                       expose_data);
            flag_default_set = TRUE;
        }
        free(choices[tindex[i]]);
    }

    if (FALSE == flag_default_set) {
        gtk_tree_model_get_iter_first(model, &iter);
        gtk_tree_view_set_cursor(
            GTK_TREE_VIEW(view), gtk_tree_path_new_from_indices(0, -1),
            NULL /* don't focus on any column */,
            FALSE /* don't start editing */);
    }

    g_object_unref(model);
    free(choices);
    free(choices_translated);
    free(tindex);

    scroll = gtk_scrolled_window_new(NULL /* create horizontal adjustement */,
                                     NULL /* create vertical adjustement */);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(vpadbox), description_box,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* padding */);
    frame = gtk_frame_new(NULL /* no label */);
    gtk_container_add(GTK_CONTAINER(frame), scroll);
    gtk_box_pack_start(GTK_BOX(vpadbox), frame, TRUE /* expand */,
                       TRUE /* fill */, 0 /* padding */);
    hpadbox = gtk_hbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(hpadbox), vpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_VPADDING);
    gtk_widget_grab_focus(view);

    register_setter(select_setter, view, q, obj);

    return DC_OK;
}

/* some SELECT questions like "countrychooser/country-name" are
 * better displayed with a tree rather than a list and this question
 * handler is meant for this purpose
 */
static int gtkhandler_select_single_tree(struct frontend * obj,
                                         struct question * q,
                                         GtkWidget * qbox)
{
    char ** choices;
    char ** choices_translated;
    int i;
    int count;
    struct frontend_question_data * data;
    char const * defval;
    int * tindex = NULL;
    gchar const * indices;
    GtkWidget * hpadbox;
    GtkWidget * vpadbox;
    GtkWidget * description_box;
    int flag_default_set = FALSE;
    GtkTreeModel * model;
    GtkTreeStore * store;
    GtkTreeIter iter;
    GtkTreeIter child;
    GtkWidget * view;
    GtkWidget * scroll;
    GtkWidget * frame;
    GtkCellRenderer * renderer;
    GtkTreeSelection * selection;
    struct treeview_expose_callback_data * expose_data;

    defval = question_getvalue(q, "");
    indices = q_get_indices(q);
    expose_data = NEW(struct treeview_expose_callback_data);

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_select_single_tree() called"); */

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0) {
        return DC_NOTOK;
    }
    choices = malloc(sizeof (char *) * count);
    choices_translated = malloc(sizeof (char *) * count);
    tindex = malloc(sizeof (int) * count);

    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q),
                           indices, choices, choices_translated,
                           tindex, count) != count) {
        return DC_NOTOK;
    }

    view = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(view), -1 /* insert at the end */,
        q_get_description(q) /* title */, renderer,
        /* column: */ "text", SELECT_COL_NAME,
        NULL /* end of column list */);
    store = gtk_tree_store_new(SELECT_NUM_COLS, G_TYPE_STRING, G_TYPE_UINT);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    g_signal_connect(G_OBJECT(view), "row-activated",
                     G_CALLBACK(select_onRowActivated), obj->data);
    g_signal_connect(G_OBJECT(view), "destroy",
                     G_CALLBACK(free_description_data), data);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), TRUE);
    gtk_tree_selection_set_mode(
        gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
        GTK_SELECTION_BROWSE);
    model = GTK_TREE_MODEL(store);
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

    for (i = 0; i < count; i++) {
        if (0 == strcmp(q->tag, "countrychooser/country-name")) {
            if ('-' == choices_translated[i][0] &&
                '-' == choices_translated[i][1]) {
                /* father, continent, will never receive focus by default*/
                gtk_tree_store_append(store, &iter, NULL /* no parents */);
                gtk_tree_store_set(
                    store, &iter,
                    /* column: */ SELECT_COL_NAME, choices_translated[i],
                    -1 /* end of list */);
            } else {
                /* child, country */
                gtk_tree_store_append(store, &child, &iter);
                gtk_tree_store_set(
                    store, &child,
                    /* column: */ SELECT_COL_NAME, choices_translated[i],
                    -1 /* end of list */);

                if (!flag_default_set && NULL != defval &&
                    0 == strcmp(choices[tindex[i]], defval)) {
                    gtk_tree_view_expand_row(
                        GTK_TREE_VIEW(view),
                        gtk_tree_model_get_path(model, &iter),
                        TRUE /* recursively open all children */);
                    expose_data->path = gtk_tree_path_to_string(
                        gtk_tree_model_get_path(model, &child));
                    expose_data->callback_function = g_signal_connect_after(
                        G_OBJECT(view), "expose_event",
                        G_CALLBACK(treeview_exposed_callback),
                        expose_data);
                    flag_default_set = TRUE;
                }
            }
        } else if (0 == strcmp(q->tag, "partman/choose_partition")) {
            if (NULL != strstr(choices_translated[i], "    ")) {
                /* child, partition */
                gtk_tree_store_append(store, &child, &iter);
                gtk_tree_store_set(
                    store, &child,
                    /* column: */ SELECT_COL_NAME, choices_translated[i],
                    -1 /* end of list */);
                gtk_tree_view_expand_row(
                    GTK_TREE_VIEW(view),
                    gtk_tree_model_get_path(model, &iter),
                    TRUE /* recursively open all children */);
                if (!flag_default_set && NULL != defval &&
                    0 == strcmp(choices[tindex[i]], defval)) {
                    expose_data->path = gtk_tree_path_to_string(
                        gtk_tree_model_get_path(model, &child));
                    expose_data->callback_function = g_signal_connect_after(
                        G_OBJECT(view), "expose_event",
                        G_CALLBACK(treeview_exposed_callback),
                        expose_data);
                    flag_default_set = TRUE;
                }
            } else {
                /* father, disk */
                gtk_tree_store_append(store, &iter, NULL /* no parents */);
                gtk_tree_store_set(
                    store, &iter,
                    /* column: */ SELECT_COL_NAME, choices_translated[i],
                    -1 /* end of list */);
                if (!flag_default_set && NULL != defval &&
                    0 == strcmp(choices[tindex[i]], defval)) {
                    expose_data->path = gtk_tree_path_to_string(
                        gtk_tree_model_get_path(model, &iter));
                    expose_data->callback_function = g_signal_connect_after(
                        G_OBJECT(view), "expose_event",
                        G_CALLBACK(treeview_exposed_callback),
                        expose_data);
                    flag_default_set = TRUE;
                }
            }
        }
        free(choices[tindex[i]]);
    }

    if (FALSE == flag_default_set) {
        gtk_tree_model_get_iter_first(model, &iter);
        gtk_tree_view_set_cursor(
            GTK_TREE_VIEW(view),
            gtk_tree_path_new_from_indices(0, -1),
            NULL /* don't focus on a particular column */,
            FALSE /* don't start editing */);
    }

    g_object_unref(model);
    free(choices);
    free(choices_translated);
    free(tindex);

    scroll = gtk_scrolled_window_new(NULL /* create horizontal adjustement */,
                                     NULL /* create vertical adjustement */);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(vpadbox), description_box,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* padding */);
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), scroll);
    gtk_box_pack_start(GTK_BOX(vpadbox), frame, TRUE /* expand */,
                       TRUE /* fill */, 0 /* padding */);
    hpadbox = gtk_hbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(hpadbox), vpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_VPADDING);
    gtk_widget_grab_focus(view);

    register_setter(select_setter, view, q, obj);

    return DC_OK;
}

static int gtkhandler_select_multiple(struct frontend * obj,
                                      struct question * q,
                                      GtkWidget * qbox)
{
    GtkWidget * description_box;
    GtkWidget * combo;
    GtkWidget * hpadbox;
    GtkWidget * vpadbox;
    GList * items = NULL;
    char ** choices;
    char ** choices_translated;
    struct frontend_question_data * data;
    int i;
    int count;
    char const * defval; 
    int * tindex = NULL;
    gchar const * indices;

    defval = question_getvalue(q, "");
    indices = q_get_indices(q);

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_select_multiple() called"); */

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0) {
        return DC_NOTOK;
    }
    choices = malloc(sizeof (char *) * count);
    choices_translated = malloc(sizeof (char *) * count);
    tindex = malloc(sizeof (int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices,
                           choices, choices_translated, tindex,
                           count) != count) {
        return DC_NOTOK;
    }
    free(choices);
    free(tindex);
    if (count <= 0) {
        return DC_NOTOK;
    }

    for (i = 0; i < count; i++) {
        items = g_list_append (items, choices_translated[i]);
        /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_select_multiple(\"%s\")",
         * choices_translated[i]); */
    }
    free(choices_translated);

    combo = gtk_combo_new();
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), items);
    g_list_free(items);
    gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(combo)->entry),
                              FALSE /* not editable */);

    if (NULL != defval) {
        gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), defval);
    } else {
        gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), "");
    }
    gtk_combo_set_value_in_list(GTK_COMBO(combo),
                                TRUE /* value must be in list */,
                                FALSE /* no empty value allowed */);

    if (is_first_question(q)) {
        gtk_widget_grab_focus(combo);
    }

    g_signal_connect(G_OBJECT(GTK_COMBO(combo)->entry), "destroy",
                     G_CALLBACK(free_description_data), data);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(vpadbox), description_box,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* padding */);
    gtk_box_pack_start(GTK_BOX(vpadbox), combo, FALSE /* don't expand */,
                       FALSE /* don't fill */, 0 /* padding */);
    hpadbox = gtk_hbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(hpadbox), vpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE /* don't expand */,
                       FALSE /* don't fill */, QUESTIONBOX_VPADDING);

    register_setter(combo_setter, GTK_COMBO(combo)->entry, q, obj);

    return DC_OK;
}

static int gtkhandler_select(struct frontend * obj, struct question * q,
                             GtkWidget * qbox)
{
    if (NULL == q->prev && NULL == q->next) {
        if (0 == strcmp(q->tag, "countrychooser/country-name") ||
            0 == strcmp(q->tag, "partman/choose_partition")) {
            return gtkhandler_select_single_tree(obj, q, qbox);
        } else {
            return gtkhandler_select_single_list(obj, q, qbox);
        }
    } else {
        return gtkhandler_select_multiple(obj, q, qbox);
    }
}

static int gtkhandler_string(struct frontend * obj, struct question * q,
                             GtkWidget * qbox)
{
    GtkWidget * description_box;
    GtkWidget * entry;
    GtkWidget * hpadbox;
    GtkWidget * vpadbox;
    struct frontend_question_data * data;
    char const * defval;
    
    defval = question_getvalue(q, "");

    /* INFO(INFO_DEBUG, "GTK_DI - gtkhandler_string() called"); */

    data = NEW(struct frontend_question_data);
    data->obj = obj;
    data->q = q;

    entry = gtk_entry_new();
    if (NULL != defval) {
        gtk_entry_set_text(GTK_ENTRY(entry), defval);
    } else {
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
    gtk_entry_set_max_length(GTK_ENTRY(entry), STRING_MAX_LENGTH);
    gtk_entry_set_activates_default(
        GTK_ENTRY(entry), TRUE /* activate on Enter key */);

    g_signal_connect(G_OBJECT(entry), "destroy",
                     G_CALLBACK(free_description_data), data);

    description_box = display_descriptions(q, obj);

    vpadbox = gtk_vbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(vpadbox), description_box,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* padding */);
    gtk_box_pack_start(GTK_BOX(vpadbox), entry, FALSE /* don't expand */,
                       FALSE /* don't fill */, 0 /* padding */);
    hpadbox = gtk_hbox_new(FALSE /* don't make children equal */,
                           DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(hpadbox), vpadbox, TRUE /* expand */,
                       TRUE /* fill */, QUESTIONBOX_HPADDING);
    gtk_box_pack_start(GTK_BOX(qbox), hpadbox, FALSE /* don't expand */,
                       FALSE /* don't fill */, QUESTIONBOX_VPADDING);
    if (is_first_question(q)) {
        gtk_widget_grab_focus(entry);
    }

    register_setter(entry_setter, entry, q, obj);

    return DC_OK;
}

static struct question_handlers {
    char const * type;
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

/* XXX: proofread and document every single constant */
static void set_design_elements(struct frontend * obj, GtkWidget * window)
{
    GtkWidget * v_mainbox;
    GtkWidget * h_mainbox;
    GtkWidget * logobox;
    GtkWidget * targetbox;
    GtkWidget * actionbox;
    GtkWidget * h_actionbox;
    GtkWidget * button_next;
    GtkWidget * button_prev;
    GtkWidget * button_screenshot;
    GtkWidget * button_cancel;
    GtkWidget * progress_bar;
    GtkWidget * progress_bar_label;
    GtkWidget * progress_bar_box;
    GtkWidget * h_progress_bar_box;
    GtkWidget * v_progress_bar_box;
    GtkWidget * label_title;
    GtkWidget * h_title_box;
    GtkWidget * v_title_box;
    GtkWidget * logo_button;
    GList * focus_chain = NULL;
    int * ret_val;
    struct frontend_data * data = obj->data;

    /* A logo is displayed in the upper area of the screen */
    /* XXX: constant */
    logo_button = gtk_image_new_from_file(LOGO_IMAGE_PATH);
    g_signal_connect_after(G_OBJECT(logo_button), "expose_event",
                           G_CALLBACK(expose_event_callback), obj);
  
    /* A label is used to display the fontend's title */
    label_title = gtk_label_new(NULL /* no label */);
    gtk_misc_set_alignment(GTK_MISC(label_title), 0, 0);
    data->title = label_title;
    h_title_box = gtk_hbox_new(TRUE /* make children equal */,
                               0 /* padding */);
    gtk_box_pack_start(GTK_BOX(h_title_box), label_title,
                       TRUE /* expand */, TRUE /* fill */, DEFAULT_PADDING);
    v_title_box = gtk_vbox_new(TRUE /* make children equal */,
                               0 /* padding */);
    gtk_box_pack_start(GTK_BOX(v_title_box), h_title_box,
                       TRUE /* expand */, TRUE /* fill */, 0 /* padding */);

    /* This is the box were question(s) will be displayed */
    targetbox = gtk_vbox_new(FALSE /* don't make children equal */, 0);
    data->target_box = targetbox;

    actionbox = gtk_hbutton_box_new();
    h_actionbox = gtk_hbox_new(FALSE /* don't make children equal */,
                               0 /* padding */);
    gtk_box_pack_start(GTK_BOX(h_actionbox), actionbox, TRUE /* expand */,
                       TRUE /* fill */, DEFAULT_PADDING);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(actionbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(actionbox), DEFAULT_PADDING);

    /* Screenshot button is set insensitive by default */
    button_screenshot = gtk_button_new_with_label(
        get_text(obj, "debconf/gtk-button-screenshot", "Screenshot"));
    g_signal_connect(G_OBJECT(button_screenshot), "clicked",
                     G_CALLBACK(screenshot_button_callback), obj);
    gtk_box_pack_start(GTK_BOX(actionbox), button_screenshot,
                       TRUE /* expand */, TRUE /* fill */, DEFAULT_PADDING);
    data->button_screenshot = button_screenshot;
    gtk_widget_set_sensitive(button_screenshot, FALSE /* insensitive */);

    /* Here are the back and forward buttons */
    button_prev = gtk_button_new_with_label(
        get_text(obj, "debconf/button-goback", "Go Back"));
    ret_val = NEW(int);
    *ret_val = DC_GOBACK;
    gtk_object_set_user_data(GTK_OBJECT(button_prev), ret_val);
    g_signal_connect(G_OBJECT(button_prev), "clicked",
                     G_CALLBACK(exit_button_callback), obj);
    gtk_box_pack_start(GTK_BOX(actionbox), button_prev,
                       TRUE /* expand */, TRUE /* fill */, DEFAULT_PADDING);

    button_next = gtk_button_new_with_label(
        get_text(obj, "debconf/button-continue", "Continue"));
    ret_val = NEW(int);
    *ret_val = DC_OK;
    gtk_object_set_user_data(GTK_OBJECT(button_next), ret_val);
    g_signal_connect(G_OBJECT(button_next), "clicked",
                     G_CALLBACK(exit_button_callback), obj);
    gtk_box_pack_start(GTK_BOX(actionbox), button_next,
                       TRUE /* expand */, TRUE /* fill */, DEFAULT_PADDING);
    GTK_WIDGET_SET_FLAGS(button_next, GTK_CAN_DEFAULT);

    data->button_prev = button_prev;
    data->button_next = button_next;
    gtk_widget_set_sensitive(button_prev, FALSE /* insensitive */);
    gtk_widget_set_sensitive(button_next, FALSE /* insensitive */);

    /* Cancel button is set insensitive by default */
    button_cancel = gtk_button_new_with_label(
        get_text(obj, "debconf/button-cancel", "Cancel"));
    ret_val = NEW(int);
    *ret_val = DC_GOBACK;
    gtk_object_set_user_data(GTK_OBJECT(button_cancel), ret_val);
    g_signal_connect(G_OBJECT(button_cancel), "clicked",
                     G_CALLBACK(cancel_button_callback), obj);
    gtk_box_pack_start(GTK_BOX(actionbox), button_cancel,
                       TRUE /* expand */, TRUE /* fill */, DEFAULT_PADDING);
    data->button_cancel = button_cancel;
    gtk_widget_set_sensitive(button_cancel, FALSE /* insensitive */);

    /* focus order inside actionbox */
    focus_chain = g_list_append(focus_chain, button_next);
    focus_chain = g_list_append(focus_chain, button_prev);
    gtk_container_set_focus_chain(GTK_CONTAINER(actionbox), focus_chain);
    g_list_free(focus_chain);

    /* Here the the progressbar is placed */
    progress_bar = gtk_progress_bar_new();
    data->progress_bar = progress_bar;  
#if GTK_CHECK_VERSION(2,6,0)
    gtk_progress_bar_set_ellipsize(GTK_PROGRESS_BAR(progress_bar),
                                   PANGO_ELLIPSIZE_MIDDLE);
#endif
    progress_bar_box = gtk_vbox_new(FALSE /* don't make children equal */,
                                    0 /* padding */);
    v_progress_bar_box = gtk_vbox_new(FALSE /* don't make children equal */,
                                      0 /* padding */);
    h_progress_bar_box = gtk_hbox_new(FALSE /* don't make children equal */,
                                      0 /* padding */);
    progress_bar_label = gtk_label_new("");
    data->progress_bar_label = progress_bar_label;
    gtk_misc_set_alignment(GTK_MISC(progress_bar_label),
                           0 /* left */, 0 /* top */);
    gtk_box_pack_start(GTK_BOX(progress_bar_box), progress_bar,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* expand */);
    gtk_box_pack_start(GTK_BOX(progress_bar_box), progress_bar_label,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(v_progress_bar_box), progress_bar_box,
                       TRUE /* expand */, TRUE /* fill */,
                       PROGRESSBAR_VPADDING);
    gtk_box_pack_start(GTK_BOX(h_progress_bar_box), v_progress_bar_box,
                       TRUE /* expand */, TRUE /* fill */,
                       PROGRESSBAR_HPADDING);
    data->progress_bar_box = h_progress_bar_box;

    /* Final packaging */
    v_mainbox = gtk_vbox_new(FALSE /* don't make children equal */,
                             0 /* padding */);
    h_mainbox = gtk_hbox_new(FALSE /* don't make children equal */,
                             0 /* padding */);
    logobox = gtk_vbox_new(FALSE /* don't make children equal */,
                           0 /* padding */);
    gtk_box_pack_start(GTK_BOX(v_mainbox), v_title_box,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* padding */);
    gtk_box_pack_start(GTK_BOX(v_mainbox), h_progress_bar_box,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(v_mainbox), targetbox, TRUE /* expand */,
                       TRUE /* fill */, DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(v_mainbox), h_actionbox,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(h_mainbox), v_mainbox, TRUE /* expand */,
                       TRUE /* fill */, DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(logobox), logo_button, FALSE /* don't expand */,
                       FALSE /* don't fill */, 0 /* padding */);
    gtk_box_pack_start(GTK_BOX(logobox), h_mainbox, TRUE /* expand */,
                       TRUE /* fill */, DEFAULT_PADDING);
    gtk_container_add(GTK_CONTAINER(window), logobox);
    
    /* pressing ESC key simulates a user's click on the "Back" button*/
    g_signal_connect_after(window, "key_press_event",
                           G_CALLBACK(key_press_event), obj);
     
}

static void * eventhandler_thread()
{
    gdk_threads_enter();
    gtk_main();
    gdk_threads_leave();

    return 0;
}

static int gtk_initialize(struct frontend * obj, struct configuration * conf)
{
    struct frontend_data * fe_data;
    GtkWidget * window;
    GThread * thread_events_listener;
    GError * err_events_listener = NULL ;
    int args = 1;
    char ** name;

    name = malloc(2 * sizeof (char *));
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

    if (!g_thread_supported()) {
       g_thread_init(NULL);
       gdk_threads_init();
    } else {
        INFO(INFO_DEBUG, "GTK_DI - gtk_initialize() failed to initialize "
                         "threads\n%s",
             err_events_listener->message);
        g_error_free(err_events_listener);
        return DC_NOTOK;
    }
    
    gtk_init(&args, &name);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    gtk_window_set_resizable(GTK_WINDOW (window), TRUE /* resizable */);
    gtk_window_set_position(GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW (window), TRUE /* resizable */);
    set_design_elements(obj, window);
    gtk_rc_reparse_all();
    fe_data->window = window;
    gtk_widget_set_default_direction(get_text_direction(obj));
    gtk_widget_show_all(window);

    button_cond = g_cond_new();
    button_mutex = g_mutex_new();

    thread_events_listener = g_thread_create(
        (GThreadFunc) eventhandler_thread, NULL /* no data */,
        TRUE /* joinable thread */, &err_events_listener);
    if (NULL == thread_events_listener) {
        INFO(INFO_DEBUG, "GTK_DI - gtk_initialize() failed to create "
                         "events listener thread\n%s",
             err_events_listener->message);
        g_error_free(err_events_listener);
        return DC_NOTOK;
    }   

    /* Workaround for bug #407035 */
    /* TODO: replace by more structural fix (or remove if fixed upstream) */
    GtkSettings * settings = gtk_settings_get_default();
    gtk_settings_set_long_property(settings, "gtk-dnd-drag-threshold",
                                   1000, "g-i");

    return DC_OK;
}

static void gtk_plugin_destroy_notify(void * data)
{
    plugin_delete((struct plugin *) data);
}

static int gtk_go(struct frontend * obj)
{
    struct frontend_data * data = (struct frontend_data *) obj->data;
    struct question * q = obj->questions;
    GtkWidget * questionbox;
    GtkWidget * questionbox_scroll = NULL;
    di_slist * plugins;
    size_t i;
    int j;
    int ret;
    gtk_handler * handler;
    struct plugin * plugin;

    if (NULL == q) {
        return DC_OK;
    }

    data->setters = NULL;

    gdk_threads_enter();

    /* TODO
     * workaround to force dfb to reload keymap at every run, awaiting
     * for dfb to support automatic keymap change detection and reloading
     * (See also bug #381979)
     */

    dfb_input_device_reload_keymap(dfb_input_device_at(DIDID_KEYBOARD));

    gtk_rc_reparse_all();

    questionbox = gtk_vbox_new(FALSE /* don't make children equal */,
                               0 /* padding */);

    /* since all widgets used to display single questions have native
     * scrolling capabilities or do not need scrolling since they're
     * usually small they can manage scrolling be autonomously.
     * vice-versa the most simple approach when displaying multiple questions
     * togheter (whose handling wigets haven't native scrolling capabilities)
     * is to pack them all inside a viewport
     */
    if (NULL == obj->questions->next && NULL == obj->questions->prev) {
        gtk_box_pack_start(GTK_BOX(data->target_box), questionbox,
                           TRUE /* expand */, TRUE /* fill */,
                           0 /* padding */);
    } else {
        questionbox_scroll = gtk_scrolled_window_new(
            NULL /* create horizontal adjustement */,
            NULL /* create vertical adjustement */);
        gtk_scrolled_window_add_with_viewport(
            GTK_SCROLLED_WINDOW(questionbox_scroll), questionbox);
        gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(questionbox_scroll),
            GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type(
            GTK_SCROLLED_WINDOW(questionbox_scroll), GTK_SHADOW_NONE);
        gtk_box_pack_start(GTK_BOX(data->target_box), questionbox_scroll,
                           TRUE /* expand */, TRUE /* fill */,
                           DEFAULT_PADDING);
    }

    /* now we can safely handle all other questions, if any */
    j = 0;
    plugins = di_slist_alloc();
    while (NULL != q) {
        j++;
        /* INFO(INFO_DEBUG, "GTK_DI - question %d: %s (type %s)", j, q->tag,
         * q->template->type); */
        for (i = 0; i < DIM(question_handlers); i++) {
            plugin = NULL;

            if (*question_handlers[i].type) {
                handler = question_handlers[i].handler;
            } else {
                plugin = plugin_find(obj, q->template->type);
                if (plugin) {
                    INFO(INFO_DEBUG, 
                         "GTK_DI - Found plugin for %s", q->template->type);
                    handler = (gtk_handler *) plugin->handler;
                    di_slist_append(plugins, plugin);
                } else {
                    INFO(INFO_DEBUG,
                         "GTK_DI - No plugin for %s", q->template->type);
                    continue;
                }
            }

            if (plugin || 0 == strcmp(q->template->type,
                                      question_handlers[i].type)) {
                ret = handler(obj, q, questionbox);
                if (ret != DC_OK) {
                    di_slist_destroy(plugins, &gtk_plugin_destroy_notify);
                    INFO(INFO_DEBUG,
                         "GTK_DI - question %d: \"%s\" failed to display!",
                         j, q->tag);
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

    if (obj->methods.can_go_back(obj, q)) {
        gtk_widget_set_sensitive (data->button_prev, TRUE);
    } else {
        gtk_widget_set_sensitive (data->button_prev, FALSE);
    }

    gtk_widget_set_sensitive(GTK_WIDGET(data->button_next),
                             TRUE /* sensitive */);
    gtk_widget_set_sensitive(data->button_screenshot, TRUE /* sensitive */);

    gtk_button_set_label(
        GTK_BUTTON(data->button_screenshot),
        get_text(obj, "debconf/gtk-button-screenshot", "Screenshot"));
    gtk_button_set_label(
        GTK_BUTTON(data->button_prev),
        get_text(obj, "debconf/button-goback", "Go Back"));
    gtk_button_set_label(
        GTK_BUTTON(data->button_next),
        get_text(obj, "debconf/button-continue", "Continue"));
    gtk_button_set_label(
        GTK_BUTTON(data->button_cancel),
        get_text(obj, "debconf/button-cancel", "Cancel"));

    gtk_widget_set_default_direction(get_text_direction(obj));

    /* The "Next" button is activated if the user presses "Enter" */
    GTK_WIDGET_SET_FLAGS(GTK_WIDGET(data->button_next), GTK_CAN_DEFAULT);
    gtk_widget_grab_default(GTK_WIDGET(data->button_next));

    update_frontend_title(obj, obj->title);

    gtk_widget_show_all(data->window);
    gtk_widget_hide(data->progress_bar_box);
    gtk_widget_hide(data->button_cancel);

    gdk_threads_leave();

    g_mutex_lock(button_mutex);
    /* frontend blocked here until the user presses either back or forward
     * button */
    g_cond_wait(button_cond, button_mutex);
    g_mutex_unlock(button_mutex);
	
    gdk_threads_enter();

    gtk_widget_set_sensitive(data->button_prev, FALSE /* insensitive */);
    gtk_widget_set_sensitive(data->button_next, FALSE /* insensitive */);
    gtk_widget_set_sensitive(data->button_screenshot, FALSE /* insensitive */);

    if (data->button_val == DC_OK) {
        call_setters(obj);
        q = obj->questions;
        while (NULL != q) {
            obj->qdb->methods.set(obj->qdb, q);
            q = q->next;
        }
    }

    di_slist_destroy(plugins, &gtk_plugin_destroy_notify);

    if (NULL == obj->questions->next && NULL == obj->questions->prev) {
        gtk_widget_destroy(questionbox);
    }
    if (NULL != questionbox_scroll) {
        gtk_widget_destroy(questionbox_scroll);
    }

    gdk_threads_leave();

    if (DC_OK == data->button_val) {
        return DC_OK;
    } else if (DC_GOBACK == data->button_val) {
        return DC_GOBACK;
    } else {
        return DC_OK;
    }
}

#if 0
static void gtk_set_title(struct frontend *obj, const char *title)
{
    /* INFO(INFO_DEBUG, "GTK_DI - gtk_set_title() called"); */

    gdk_threads_enter();
    update_frontend_title (obj, (char *)title);
    gdk_threads_leave();
}
#endif

static bool gtk_can_go_back(struct frontend * obj, struct question * q)
{
    return DCF_CAPB_BACKUP == (obj->capability & DCF_CAPB_BACKUP);
}

static bool gtk_can_cancel_progress(struct frontend * obj)
{
    return DCF_CAPB_PROGRESSCANCEL ==
               (obj->capability & DCF_CAPB_PROGRESSCANCEL);
}

static void set_design_elements_while_progressbar_runs(struct frontend * obj)
{
    struct frontend_data * data = (struct frontend_data *) obj->data;

    /* cancel button has to be displayed */
    if (obj->methods.can_cancel_progress(obj)) {
        gtk_widget_hide(data->button_screenshot);
        gtk_widget_hide(data->button_prev);
        gtk_widget_hide(data->button_next);
        gtk_widget_show(data->button_cancel);
        gtk_widget_set_sensitive(data->button_cancel, TRUE /* sensitive */);
        GTK_WIDGET_SET_FLAGS(GTK_WIDGET(data->button_cancel), GTK_CAN_DEFAULT);
        gtk_widget_grab_default(GTK_WIDGET(data->button_cancel));    
    } else {
        gtk_widget_hide(data->button_screenshot);
        gtk_widget_hide(data->button_prev);
        gtk_widget_hide(data->button_next);
        gtk_widget_hide(data->button_cancel);
    }

    gtk_widget_show(data->progress_bar_box);
}

static void gtk_progress_start(struct frontend * obj, int min, int max,
                               char const * title)
{
    struct frontend_data * data = (struct frontend_data *) obj->data;
    GtkWidget *progress_bar;

    gdk_threads_enter();
    gtk_rc_reparse_all();
    gtk_button_set_label(GTK_BUTTON(data->button_screenshot),
        get_text(obj, "debconf/gtk-button-screenshot", "Screenshot"));
    gtk_button_set_label(GTK_BUTTON(data->button_prev),
        get_text(obj, "debconf/button-goback", "Go Back"));
    gtk_button_set_label(GTK_BUTTON(data->button_next),
        get_text(obj, "debconf/button-continue", "Continue"));
    gtk_button_set_label(GTK_BUTTON(data->button_cancel),
        get_text(obj, "debconf/button-cancel", "Cancel"));
    gtk_widget_set_default_direction(get_text_direction(obj));
    set_design_elements_while_progressbar_runs(obj);
    DELETE(obj->progress_title);
    obj->progress_title=strdup(title);
    progress_bar = data->progress_bar;
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar),
                              obj->progress_title);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar),
                                  0.0 /* empty progress bar */);
    obj->progress_min = min;
    obj->progress_max = max;
    obj->progress_cur = min;
    
    free(progressbar_title);
    /* XXX */
    progressbar_title = malloc(strlen(obj->title) + 1);
    strcpy(progressbar_title, obj->title);
    update_frontend_title(obj, progressbar_title);
    
    gdk_threads_leave();

    /* INFO(INFO_DEBUG, "GTK_DI - gtk_progress_start(min=%d, max=%d, title=%s)
     * called", min, max, title); */
}

static int gtk_progress_set(struct frontend * obj, int val)
{
    struct frontend_data * data = (struct frontend_data *) obj->data;
    gdouble progress;
    GtkWidget * progress_bar;

    gdk_threads_enter();
    set_design_elements_while_progressbar_runs(obj);
    
    /* INFO(INFO_DEBUG, "GTK_DI - gtk_progress_set(val=%d) called", val); */

    update_frontend_title(obj, progressbar_title);
    progress_bar = data->progress_bar;
    gtk_widget_set_sensitive(GTK_WIDGET(progress_bar), TRUE /* sensitive */);

    obj->progress_cur = val;
    if ((obj->progress_max - obj->progress_min) > 0) {
        /* XXX: mh... is there a better way than plain cast? */
        progress = (gdouble) (obj->progress_cur - obj->progress_min) /
                   (gdouble) (obj->progress_max - obj->progress_min);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar),
                                      progress);
    }
    gdk_threads_leave();

    if (DC_OK == data->button_val || DC_GOBACK == data->button_val) {
        return data->button_val;
    } else {
        return DC_OK;
    }
}

static int gtk_progress_info(struct frontend * obj, const char * info)
{
    struct frontend_data * data = (struct frontend_data *) obj->data;
    GtkWidget * progress_bar_label;
    char * progress_bar_label_string;

    gdk_threads_enter();
    set_design_elements_while_progressbar_runs(obj);

    /* INFO(INFO_DEBUG, "GTK_DI - gtk_progress_info(%s) called", info); */

    update_frontend_title(obj, progressbar_title);
    progress_bar_label = data->progress_bar_label;
    /* XXX! */
    progress_bar_label_string = malloc(strlen(info) + 10);
    sprintf(progress_bar_label_string,"<i> %s</i>",info);

    gtk_label_set_markup(GTK_LABEL(progress_bar_label),
                         progress_bar_label_string);
    free(progress_bar_label_string);
    gdk_threads_leave();

    if (DC_OK == data->button_val || DC_GOBACK == data->button_val) {
        return data->button_val;
    } else {
        return DC_OK;
    }
}

static void gtk_progress_stop(struct frontend *obj)
{
    struct frontend_data * data = (struct frontend_data *) obj->data;

    /* INFO(INFO_DEBUG, "GTK_DI - gtk_progress_stop() called"); */
    gdk_threads_enter();
    gtk_widget_hide(data->progress_bar_box);
    gtk_widget_hide(data->button_cancel);
    gdk_threads_leave();
}

static unsigned long gtk_query_capability(struct frontend * f)
{
    /* INFO(INFO_DEBUG, "GTK_DI - gtk_query_capability() called"); */
    return DCF_CAPB_BACKUP;
}

struct frontend_module debconf_frontend_module = {
    initialize: gtk_initialize,
    go: gtk_go,
/*  set_title: gtk_set_title, */
    can_go_back: gtk_can_go_back,
    can_cancel_progress: gtk_can_cancel_progress,
    progress_start: gtk_progress_start,
    progress_info: gtk_progress_info,
    progress_set: gtk_progress_set,
    progress_stop: gtk_progress_stop,
    query_capability: gtk_query_capability,
};

void update_frontend_title(struct frontend * obj, char * title)
{
    struct frontend_data * data = (struct frontend_data *) obj->data;
    char * tmp;

    /* XXX! */
    tmp = malloc(strlen (title) + 12);
    sprintf (tmp,"<b> %s</b>", title);
    gtk_label_set_markup(GTK_LABEL(data->title), tmp);
    free(tmp);
}

/* vim: et sw=4 si
 */
