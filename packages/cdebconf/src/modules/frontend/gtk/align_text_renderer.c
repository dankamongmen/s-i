/*****************************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * $Id$
 *
 * cdebconf is (c) 2000-2007 Randolph Chung and others under the following
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
 *****************************************************************************/

/** @file align_text_renderer.c
 * GtkCellRenderer for text aligned with tab stops
 */

#include "align_text_renderer.h"

enum {
    PROP_0,

    PROP_TEXT
};

static gpointer cdebconf_gtk_align_text_renderer_parent_class;

static void cdebconf_gtk_align_text_renderer_init(AlignTextRenderer * renderer)
{
    GTK_CELL_RENDERER(renderer)->xalign = 0.0;
    GTK_CELL_RENDERER(renderer)->yalign = 0.5;
    GTK_CELL_RENDERER(renderer)->xpad = 2;
    GTK_CELL_RENDERER(renderer)->ypad = 2;
    renderer->tab_array = NULL;
}

static void align_text_renderer_finalize(GObject * object)
{
    AlignTextRenderer * renderer = ALIGN_TEXT_RENDERER(object);

    if (NULL != renderer->text) {
        g_free(renderer->text);
    }
    if (NULL != renderer->tab_array) {
        pango_tab_array_free(renderer->tab_array);
    }

    (* G_OBJECT_CLASS(
        cdebconf_gtk_align_text_renderer_parent_class)->finalize)(object);
}

static void align_text_renderer_get_property(
    GObject * object, guint param_id, GValue * value,
    GParamSpec * param_spec)
{
    AlignTextRenderer * renderer = ALIGN_TEXT_RENDERER(object);

    switch (param_id) {
        case PROP_TEXT:
            g_value_set_string(value, renderer->text);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, param_spec);
            break;
    }
}

static void align_text_renderer_set_property(
    GObject * object, guint param_id, const GValue * value,
    GParamSpec * param_spec)
{
    AlignTextRenderer * renderer = ALIGN_TEXT_RENDERER(object);

    switch (param_id) {
        case PROP_TEXT:
            if (NULL != renderer->text) {
                g_free(renderer->text);
            }
            renderer->text = g_strdup(g_value_get_string(value));
            g_object_notify(object, "text");
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, param_spec);
            break;
    }
}

void cdebconf_gtk_align_text_renderer_set_tab_array(
    AlignTextRenderer * renderer, PangoTabArray * tab_array)
{
    if (NULL != renderer->tab_array) {
        pango_tab_array_free(renderer->tab_array);
    }
    renderer->tab_array = pango_tab_array_copy(tab_array);
}

GtkCellRenderer * cdebconf_gtk_align_text_renderer_new(void)
{
    return g_object_new(TYPE_ALIGN_TEXT_RENDERER, NULL);
}

static PangoLayout * get_layout(AlignTextRenderer * renderer,
                                GtkWidget * widget)
{
    PangoLayout * layout;

    layout = gtk_widget_create_pango_layout(widget, renderer->text);
    pango_layout_set_width(layout, -1);
    pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
    /* XXX? pango_layout_set_alignment */
    pango_layout_set_tabs(layout, renderer->tab_array);
    return layout;
}

static void align_text_renderer_get_size(
    GtkCellRenderer * cell, GtkWidget * widget, GdkRectangle * cell_area,
    gint * x_offset, gint * y_offset, gint * width, gint * height)
{
    AlignTextRenderer * renderer = ALIGN_TEXT_RENDERER(cell);
    PangoLayout * layout;
    PangoRectangle rect;

    layout = get_layout(renderer, widget);
    pango_layout_get_pixel_extents(layout, NULL, &rect);
    if (NULL != height) {
        *height = cell->ypad * 2 + rect.height;
    }
    if (NULL != width) {
        *width = cell->xpad * 2 + rect.x + rect.width;
    }

    if (NULL != cell_area) {
        if (NULL != x_offset) {
            *x_offset = cell_area->width - rect.x - rect.width -
                        (2 * cell->xpad);
            if (GTK_TEXT_DIR_RTL == gtk_widget_get_direction(widget)) {
                *x_offset *= 1.0 - cell->xalign;
            } else {
                *x_offset *= cell->xalign;
            }
        }
        if (NULL != y_offset) {
            *y_offset = cell_area->height - rect.height - (2 * cell->ypad);
            *y_offset = MAX(*y_offset * cell->yalign, 0);
        }
    }
}

static GtkStateType get_state(GtkCellRenderer * cell, GtkWidget * widget,
                              GtkCellRendererState flags)
{
    if (!cell->sensitive) {
        return GTK_STATE_INSENSITIVE;
    }
    if (GTK_CELL_RENDERER_SELECTED == (flags & GTK_CELL_RENDERER_SELECTED)) {
        return GTK_WIDGET_HAS_FOCUS(widget) ?
                   GTK_STATE_SELECTED : GTK_STATE_ACTIVE;
    }
    if (GTK_CELL_RENDERER_PRELIT == (flags & GTK_CELL_RENDERER_PRELIT) &&
        GTK_STATE_PRELIGHT == GTK_WIDGET_STATE(widget)) {
        return GTK_STATE_PRELIGHT;
    }
    if (GTK_STATE_INSENSITIVE == GTK_WIDGET_STATE(widget)) {
        return GTK_STATE_INSENSITIVE;
    }
    return GTK_STATE_NORMAL;
}

static void align_text_renderer_render(
    GtkCellRenderer * cell, GdkWindow * window, GtkWidget * widget,
    GdkRectangle * background_area, GdkRectangle * cell_area,
    GdkRectangle * expose_area, guint flags)
{
    AlignTextRenderer * renderer = ALIGN_TEXT_RENDERER(cell);
    PangoLayout * layout;
    GtkStateType state;
    gint x_offset;
    gint y_offset;

    layout = get_layout(renderer, widget);
    align_text_renderer_get_size(cell, widget, cell_area, &x_offset,
                                 &y_offset, NULL, NULL);

    state = get_state(cell, widget, flags);
    gtk_paint_layout(widget->style, window, state, TRUE, expose_area,
                     widget, "align_text_renderer",
                     cell_area->x + x_offset + cell->xpad,
                     cell_area->y + y_offset + cell->ypad,
                     layout);
}

static void cdebconf_gtk_align_text_renderer_class_init(
    AlignTextRendererClass * klass)
{
    GObjectClass * object_class = G_OBJECT_CLASS(klass);
    GtkCellRendererClass * cell_class = GTK_CELL_RENDERER_CLASS(klass);

    cdebconf_gtk_align_text_renderer_parent_class =
        g_type_class_peek_parent (klass);
    object_class->finalize = align_text_renderer_finalize;
    object_class->get_property = align_text_renderer_get_property;
    object_class->set_property = align_text_renderer_set_property;

    cell_class->get_size = align_text_renderer_get_size;
    cell_class->render = align_text_renderer_render;

    klass->set_tab_array = cdebconf_gtk_align_text_renderer_set_tab_array;

    g_object_class_install_property(
        object_class, PROP_TEXT,
        g_param_spec_string("text", "Text", "Text to render", NULL,
        G_PARAM_READWRITE));
}

GType cdebconf_gtk_align_text_renderer_get_type(void)
{
    static const GTypeInfo align_text_renderer_info = {
        sizeof (AlignTextRendererClass),
        NULL,                                                     /* base_init */
        NULL,                                                     /* base_finalize */
        (GClassInitFunc) cdebconf_gtk_align_text_renderer_class_init,
        NULL,                                                     /* class_finalize */
        NULL,                                                     /* class_data */
        sizeof (AlignTextRenderer),
        0,                                                        /* n_preallocs */
        (GInstanceInitFunc) cdebconf_gtk_align_text_renderer_init,
        NULL
    };
    static GType align_text_renderer_type = 0;

    if (align_text_renderer_type) {
        return align_text_renderer_type;
    }

    /* Derive from GtkCellRenderer */
    align_text_renderer_type = g_type_register_static(
        GTK_TYPE_CELL_RENDERER, "AlignTextRenderer",
        &align_text_renderer_info, 0);
    return align_text_renderer_type;
}

/* vim: et sw=4 si
 */
