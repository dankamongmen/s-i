/*
 * cdebconf gtk plugin to display a terminal
 *
 * Copyright © 2008 Jérémy Bobbio <lunar@debian.org>
 * See debian/copyright for license.
 *
 */

#include <cdebconf/frontend.h>
#include <cdebconf/cdebconf_gtk.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <vte/vte.h>
#include <string.h>

extern char ** environ;

#define DEFAULT_COMMAND_LINE "/bin/sh"

/* Here's the plugin! */
int cdebconf_gtk_handler_terminal(struct frontend * fe,
                                  struct question * question,
                                  GtkWidget * question_box);

struct terminal {
    struct frontend * fe;
    GtkWidget * goback_button;
    VteTerminal * terminal;
    char * command;
    char ** argv;
    char ** environ;
};

static void destroy_terminal(struct terminal * terminal_data)
{
    if (NULL != terminal_data->command) {
        g_free(terminal_data->command);
    }
    if (NULL != terminal_data->argv) {
        g_strfreev(terminal_data->argv);
    }
    if (NULL != terminal_data->environ) {
        g_strfreev(terminal_data->environ);
    }
    if (NULL != terminal_data->terminal) {
        g_object_unref(G_OBJECT(terminal_data->terminal));
    }
    if (NULL != terminal_data->goback_button) {
        g_object_unref(G_OBJECT(terminal_data->goback_button));
    }
    g_free(terminal_data);
}

static void cleanup(GtkWidget * widget, struct terminal * terminal_data)
{
    destroy_terminal(terminal_data);
}

static void handle_child_exit(GtkWidget * button,
                              struct terminal * terminal_data)
{
    cdebconf_gtk_set_answer_ok(terminal_data->fe);
}

static struct terminal * init_terminal(struct frontend * fe,
                                       struct question * question)
{
    struct terminal * terminal_data;

    if (NULL == (terminal_data = g_malloc0(sizeof (struct terminal)))) {
        g_critical("g_malloc0 failed.");
        return NULL;
    }
    terminal_data->fe = fe;

    return terminal_data;
}

static gboolean init_command(struct terminal * terminal_data,
                             struct question * question)
{
    const char * command_line;

    command_line = question_get_variable(question, "COMMAND_LINE");
    if (NULL == command_line) {
        command_line = DEFAULT_COMMAND_LINE;
    }
    terminal_data->argv = g_strsplit_set(
        command_line, " \t\n" /* default IFS */,
        4096 /* max number of arguments */
        /* XXX: replace with the correct system macro */);
    if (NULL == terminal_data->argv || NULL == terminal_data->argv[0]) {
        g_critical("g_strsplit_set failed.");
        return FALSE;
    }
    terminal_data->command = g_strdup(terminal_data->argv[0]);
    if (NULL == terminal_data->command) {
        g_critical("g_strplit_set failed.");
        return FALSE;
    }
    return TRUE;
}

/* XXX: mostly copied from ui.c */
static GtkWidget * create_dialog_action_box(struct frontend * fe,
                                            GtkWidget * dialog)
{
    GtkWidget * action_box;
    GtkWidget * cancel_button;
    GtkWidget * continue_button;
    char * label;

    /* check NULL! */
    action_box = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(action_box), GTK_BUTTONBOX_END);

    label = cdebconf_gtk_get_text(fe, "debconf/button-cancel", "Cancel");
    /* check NULL! */
    cancel_button = gtk_button_new_with_label(label);
    g_free(label);

    GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
    g_signal_connect_swapped(G_OBJECT(cancel_button), "clicked",
                             G_CALLBACK(gtk_widget_destroy), dialog);
    g_signal_connect(G_OBJECT(cancel_button), "realize",
                     G_CALLBACK(gtk_widget_grab_focus), NULL /* no data */);

    label = cdebconf_gtk_get_text(fe, "debconf/button-continue", "Continue");
    continue_button = gtk_button_new_with_label(label);
    g_free(label);

    g_signal_connect_swapped(G_OBJECT(continue_button), "clicked",
                             G_CALLBACK(cdebconf_gtk_set_answer_ok), fe);
    g_signal_connect_swapped(G_OBJECT(continue_button), "clicked",
                             G_CALLBACK(gtk_widget_destroy), dialog);

    gtk_box_pack_start(GTK_BOX(action_box), cancel_button,
                       TRUE /* expand */, TRUE /* fill */, DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(action_box), continue_button,
                       TRUE /* expand */, TRUE /* fill */, DEFAULT_PADDING);

    gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(action_box),
                                       cancel_button, TRUE /* secondary */);
    return action_box;
}

/* XXX: copied from ui.c */
static GtkWidget * create_dialog_title_label(const gchar * title)
{
    GtkWidget * label;
    gchar * markup;

    /* check NULL! */
    label = gtk_label_new(NULL /* no text */);
    gtk_misc_set_alignment(GTK_MISC(label), 0 /* left aligned */,
                           0 /* top aligned */);

    markup = g_strdup_printf("<b>%s</b>", title);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);

    return label;
}

/* XXX: mostly copied from cdebconf_gtk_run_message_dialog() */
static gboolean run_confirm_dialog(struct terminal * terminal_data)
{
    GtkWidget * parent;
    GtkWidget * dialog;
    GtkWidget * vbox;
    GtkWidget * label;
    GtkWidget * frame;
    gchar * text;

    parent = gtk_widget_get_ancestor(terminal_data->goback_button,
                                     GTK_TYPE_WINDOW);

    dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE /* modal */);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE /* not resizale */);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(dialog), FALSE /* no decoration */);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 0 /* no border */);

    vbox = gtk_vbox_new(FALSE /* don't make children equal */,
                        DEFAULT_PADDING);
    text = cdebconf_gtk_get_text(
        terminal_data->fe, "debconf/terminal/gtk/confirm-title",
        "Resume installation");
    gtk_box_pack_start(GTK_BOX(vbox), create_dialog_title_label(text),
                       FALSE /* don't expand */, FALSE /* don't fille */,
                       0 /* no padding */);
    g_free(text);
    text = cdebconf_gtk_get_text(
        terminal_data->fe, "debconf/terminal/gtk/confirm-message",
        "Choose \"Continue\" to really exit the shell and resume the "
        "installation; any processes still running in the shell will be "
        "aborted.");
    label = gtk_label_new(text);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    g_free(text);
    gtk_box_pack_start(GTK_BOX(vbox), label,
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       DEFAULT_PADDING);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_hseparator_new(),
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* no padding */);
    gtk_box_pack_start(GTK_BOX(vbox),
                       create_dialog_action_box(terminal_data->fe, dialog),
                       FALSE /* don't expand */, FALSE /* don't fill */,
                       0 /* no padding */);
    cdebconf_gtk_center_widget(&vbox, DEFAULT_PADDING, DEFAULT_PADDING);

    frame = gtk_frame_new(NULL /* no label */);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_container_add(GTK_CONTAINER(dialog), frame);

    gtk_widget_show_all(dialog);
    return TRUE;
}

static GtkWidget * create_goback_button(struct terminal * terminal_data)
{
    GtkWidget * button;
    char * label;

    label = cdebconf_gtk_get_text(terminal_data->fe, "debconf/button-goback", "Go Back");
    /* XXX: check NULL! */
    button = gtk_button_new_with_label(label);
    g_free(label);

    g_signal_connect_swapped(G_OBJECT(button), "clicked",
                             G_CALLBACK(run_confirm_dialog), terminal_data);

    cdebconf_gtk_add_button(terminal_data->fe, button);

    return button;
}

static gboolean handle_goback_key(GtkWidget * widget, GdkEventKey * key,
                                  struct terminal * terminal_data)
{
    if (GDK_Escape == key->keyval &&
        (GDK_SHIFT_MASK | GDK_CONTROL_MASK) ==
        (key->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))) {
        run_confirm_dialog(terminal_data);
        return TRUE;
    }
    return FALSE;
}

static GtkWidget * create_widgets(struct terminal * terminal_data)
{
    GtkWidget * hbox;
    VteTerminal * terminal;
    GtkWidget * scrollbar;
    GtkWidget * goback_button;

    goback_button = create_goback_button(terminal_data);
    if (NULL == goback_button) {
        g_critical("create_goback_button failed.");
        return NULL;
    }
    g_object_ref(G_OBJECT(goback_button));
    terminal_data->goback_button = goback_button;

    g_setenv("VTE_BACKEND", "pango", TRUE /* overwrite */);
    if (NULL == (terminal = VTE_TERMINAL(vte_terminal_new()))) {
        g_critical("vte_terminal_new failed.");
        return NULL;
    }
    vte_terminal_set_font_from_string(terminal, "monospace");
    g_signal_connect(terminal, "destroy", G_CALLBACK(cleanup), terminal_data);
    g_signal_connect(terminal, "child-exited", G_CALLBACK(handle_child_exit),
                     terminal_data);
    g_signal_connect(terminal, "key_press_event", G_CALLBACK(handle_goback_key),
                     terminal_data);
    g_signal_connect(terminal, "realize", G_CALLBACK(gtk_widget_grab_focus),
                     NULL);
    g_object_ref(terminal);
    terminal_data->terminal = terminal;

    hbox = gtk_hbox_new(FALSE /* not homogeneous */, 0 /* no spacing */);
    if (NULL == hbox) {
        g_critical("gtk_hbox_new failed.");
        return NULL;
    }
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(terminal), TRUE /* expand */,
                       TRUE /* fill */, 0 /* no padding */);

    scrollbar = gtk_vscrollbar_new(terminal->adjustment);
    if (NULL == scrollbar) {
        g_critical("gtk_vscrollbar_new failed.");
        return NULL;
    }
    gtk_box_pack_start(GTK_BOX(hbox), scrollbar, FALSE /* don't expand */,
                       FALSE /* don't fill */, 0 /* no padding */);
    return hbox;
}

static void set_nothing(struct question * question, void * dummy)
{
    /* terminal questions do not put anything in the database */
    return;
}

static gboolean prepare_environ(struct terminal * terminal_data)
{
    guint orig_index;
    guint new_index;
    const char * src;

    terminal_data->environ = g_new0(char *, g_strv_length(environ) + 1);
    if (NULL == terminal_data->environ) {
        g_critical("g_malloc0 failed.");
        return FALSE;
    }
    new_index = 0;
    for (orig_index = 0; NULL != environ[orig_index]; orig_index++) {
        if (g_str_has_prefix(environ[orig_index], "DEBIAN_HAS_FRONTEND=")) {
            src = "DEBIAN_HAS_FRONTEND=";
        } else if (g_str_has_prefix(environ[orig_index], "DEBIAN_FRONTEND=")) {
            src = "DEBIAN_FRONTEND=newt";
        } else {
            src = environ[orig_index];
        }
        if (NULL == (terminal_data->environ[new_index] = g_strdup(src))) {
            g_critical("g_strdup failed.");
            return FALSE;
        }
        new_index++;
    }
    return TRUE;
}

static gboolean start_command(struct terminal * terminal_data)
{
    pid_t pid;

    pid = vte_terminal_fork_command(
        terminal_data->terminal, terminal_data->command,
        terminal_data->argv, terminal_data->environ, "/",
        FALSE /* no lastlog */, FALSE /* no utmp */, FALSE /* no wtmp */);
    if (0 == pid) {
        g_critical("vte_terminal_fork_command failed.");
        return FALSE;
    }
    return TRUE;
}

int cdebconf_gtk_handler_terminal(struct frontend * fe,
                                  struct question * question,
                                  GtkWidget * question_box)
{
    struct terminal * terminal_data;
    GtkWidget * widget;

    if (!IS_QUESTION_SINGLE(question)) {
        g_critical("entropy plugin does not work alongside other questions.");
        return DC_NOTOK;
    }
    if (NULL == (terminal_data = init_terminal(fe, question))) {
        g_critical("init_terminal failed.");
        return DC_NOTOK;
    }
    if (NULL == (widget = create_widgets(terminal_data))) {
        g_critical("create_widgets failed.");
        goto failed;
    }
    if (!init_command(terminal_data, question)) {
        g_critical("init_command failed.");
        goto failed;
    }
    if (!prepare_environ(terminal_data)) {
        g_critical("prepare_environ failed.");
        goto failed;
    }
    if (!start_command(terminal_data)) {
        g_critical("start_command failed.");
        goto failed;
    }

    cdebconf_gtk_add_common_layout(fe, question, question_box, widget);

    cdebconf_gtk_register_setter(fe, SETTER_FUNCTION(set_nothing), question,
                                 NULL);

    return DC_OK;

failed:
    destroy_terminal(terminal_data);
    return DC_NOTOK;
}

/* vim: et sw=4 si
 */
