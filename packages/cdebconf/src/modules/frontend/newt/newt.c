/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: newt.c
 *
 * Description: Newt UI for cdebconf
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

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <wchar.h>
#include <assert.h>

#include <slang.h>
#include <newt.h>

#ifdef HAVE_LIBTEXTWRAP
#include <textwrap.h>
#endif

struct newt_data {
    newtComponent scale_form,
                  scale_bar,
                  scale_textbox,
                  perc_label;
    int           scale_textbox_height;
};

typedef int (newt_handler)(struct frontend *obj, struct question *q);

#define q_get_extended_description(q)   question_get_field((q), "", "extended_description")
#define q_get_description(q)  		question_get_field((q), "", "description")
#define q_get_choices(q)		question_get_field((q), "", "choices")
#define q_get_choices_vals(q)		question_get_field((q), NULL, "choices")
#define q_get_indices(q)		question_get_field((q), "", "indices")

#define create_form(scrollbar)          newtForm((scrollbar), NULL, 0)

/*  Horizontal offset between buttons and text box */
#define TEXT_PADDING 1
/*  Horizontal offset between text box and borders */
#define BUTTON_PADDING 4

/*
 * Function: get_text
 * Input: struct frontend *obj - frontend object
 *        const char *template - template name
 *        const char *fallback - string to use if not available
 * Output: const char * - ptr to string, translated if possible
 * Description: get the translated version of a string
 * Assumptions: None.
 */
static const char *
get_text(struct frontend *obj, const char *template, const char *fallback )
{
	struct question *q = obj->qdb->methods.get(obj->qdb, template);
	return q ? q_get_description(q) : fallback;
}

/* gettext would be much nicer :-( */
static const char *
continue_text(struct frontend *obj)
{
    return get_text(obj, "debconf/button-continue", "Continue");
}

static const char *
goback_text(struct frontend *obj)
{
    return get_text(obj, "debconf/button-goback", "Go Back");
}

static const char *
yes_text(struct frontend *obj)
{
    return get_text(obj, "debconf/button-yes", "Yes");
}

static const char *
no_text(struct frontend *obj)
{
    return get_text(obj, "debconf/button-no", "No");
}

static const char *
help_text(struct frontend *obj)
{
    return get_text(obj, "debconf/help-line", "<Tab> moves between items; <Space> selects; <Enter> activates buttons");
}

static void
create_window(const int width, const int height, const char *title, const char *priority)
{
    static const char *sigils[][2] = {
        { "low",      "." },
        { "medium",   "?" },
        { "high",     "!" },
        { "critical", "!!" },
        { NULL, NULL }
    };
    char *buf = NULL;
    int i = -1;

    if (priority != NULL) {
        for (i = 0; sigils[i][0] != NULL; i++) {
            if (strcmp(priority, sigils[i][0]) == 0)
                break;
        }
        if (sigils[i][0] != NULL)
            if (asprintf(&buf, "[%s] %s", sigils[i][1], title) == -1)
                buf = NULL;
    }
    if (buf != NULL) {
        newtCenteredWindow(width, height, buf);
        free(buf);
    } else {
        newtCenteredWindow(width, height, title);
    }
}

static int
get_text_height(const char *text, int win_width)
{
    newtComponent textbox, f;
    int t_height;
#ifdef HAVE_LIBTEXTWRAP
    textwrap_t tw;
    char *wrappedtext;

    textwrap_init(&tw);
    textwrap_columns(&tw, win_width - 2 - 2*TEXT_PADDING);
    wrappedtext = textwrap(&tw, text);
    textbox = newtTextbox(TEXT_PADDING, 2, win_width - 2 - 2*TEXT_PADDING, 10, NEWT_FLAG_SCROLL);
#else

    // Create a dummy testbox to find out how many lines the text in the
    // question will occupy
    textbox = newtTextbox(TEXT_PADDING, 2, win_width - 2 - 2*TEXT_PADDING, 10, NEWT_FLAG_SCROLL|NEWT_FLAG_WRAP);
#endif
    assert(textbox);
    assert(text);
#ifdef HAVE_LIBTEXTWRAP
    newtTextboxSetText(textbox, wrappedtext);
    free(wrappedtext);
#else
    newtTextboxSetText(textbox, text);
#endif
    t_height = newtTextboxGetNumLines(textbox);
    // This is needed so the textbox gets freed...ick
    f = create_form(NULL);
    newtFormAddComponent(f, textbox);
    newtFormDestroy(f);
    return t_height;
}

static int
get_text_width(const char *text)
{
    int t_width = 0;
    const char *p = text;
    size_t res;
    int k;
    wchar_t c;

    p = text;
    do {
        for (res = 0; (k = mbtowc (&c, p, MB_LEN_MAX)) > 0 && c != L'\n'; p += k)
            res += wcwidth (c);
        if (res > t_width)
            t_width = res;
        if (*p)
            p++;
    } while (*p);

    return t_width;
}

static int
min_window_height(struct question *q, int win_width)
{
    int height = 3;
    char *type = q->template->type;
    char *q_ext_text;

    q_ext_text = q_get_extended_description(q);
    if (q_ext_text != NULL)
        height = get_text_height(q_ext_text, win_width) + 1;
    if (strcmp(type, "multiselect") == 0 || strcmp(type, "select") == 0)
        height += 4; // at least three lines for choices + blank line
    else if (strcmp(type, "string") == 0 || strcmp(type, "password") == 0)
        height += 2; // input line + blank line
    // the others don't need more space
    return height;
}

static int
need_separate_window(struct question *q)
{
    int width = 80, height = 24;
    int x;

    newtGetScreenSize(&width, &height);
    x = min_window_height(q, width-7);
    return (x >= height-5);
}

static char *
get_full_description(struct question *q)
{
    char *res = NULL;
    char *descr = q_get_description(q);
    char *ext_descr = q_get_extended_description(q);

    assert(descr);
    assert(ext_descr);
    res = malloc(strlen(descr)+strlen(ext_descr)+3);
    *res = '\0';
    if (*ext_descr) {
        strcpy(res, ext_descr);
        strcat(res, "\n\n");
    }
    strcat(res, descr);
    free(descr);
    free(ext_descr);
    return res;
}

static int
show_separate_window(struct frontend *obj, struct question *q)
{
    newtComponent form, textbox, bOk, bCancel, cRet;
    int width = 80, height = 24, t_height, t_width, win_width, win_height;
    int t_width_scroll = 0, t_width_title, t_width_descr, t_width_buttons;
    // buttons
    int extra = 3;
    int format_note = 0;
    char *full_description;
    int ret;
#ifdef HAVE_LIBTEXTWRAP
    int flags = 0;
    textwrap_t tw;
    char *wrappedtext;
#else
    int flags = NEWT_FLAG_WRAP;
#endif
    char *descr = q_get_description(q);
    char *ext_descr = q_get_extended_description(q);

    assert(descr);
    assert(ext_descr);

    if (strcmp(q->template->type, "note") == 0 || strcmp(q->template->type, "error") == 0)
    {
        format_note = 1;
        extra++;
        full_description = strdup(ext_descr);
    }
    else
        full_description = get_full_description(q);

    newtGetScreenSize(&width, &height);
    win_width = width-7;
    /*  There are 5 characters for sigils, plus 4 for borders */
    strtruncate(obj->title, win_width-9);
    t_height = get_text_height(full_description, win_width);
    if (t_height+extra <= height-5)
        win_height = t_height+extra;
    else {
        win_height = height-5;
        flags |= NEWT_FLAG_SCROLL;
        t_width_scroll = 2;
    }
    t_height = win_height-extra;
#ifdef HAVE_LIBTEXTWRAP
    textwrap_init(&tw);
    textwrap_columns(&tw, win_width - 2 - 2*TEXT_PADDING);
    wrappedtext = textwrap(&tw, full_description);
    free(full_description);
    full_description = wrappedtext;
#endif
    t_width = get_text_width(full_description);
    t_width_descr = get_text_width(descr);
    if (t_width_descr > t_width)
        t_width = t_width_descr;
    t_width_buttons = 2*BUTTON_PADDING + get_text_width(continue_text(obj)) + 2;
    if (obj->methods.can_go_back(obj, q))
        //  Add an interspace
        t_width_buttons += get_text_width(goback_text(obj)) + 3;
    if (t_width_buttons > t_width)
        t_width = t_width_buttons;
    if (win_width > t_width + 2*TEXT_PADDING + t_width_scroll)
        win_width = t_width + 2*TEXT_PADDING + t_width_scroll;
    t_width_title = get_text_width(obj->title);
    if (t_width_title > win_width)
        win_width = t_width_title;
    create_window(win_width, win_height, obj->title, q->priority);
    form = create_form(NULL);
    if (format_note)
        newtFormAddComponent(form, newtLabel((win_width - strwidth(descr))/2, 0, descr));
    textbox = newtTextbox(TEXT_PADDING, 1, t_width, t_height, flags);
    assert(textbox);
    newtTextboxSetText(textbox, full_description);
    free(full_description);
    if (obj->methods.can_go_back(obj, q)) {
        bOk     = newtCompactButton(win_width - TEXT_PADDING - BUTTON_PADDING - strwidth(continue_text(obj)) - 3, win_height-2, continue_text(obj));
        bCancel = newtCompactButton(TEXT_PADDING + BUTTON_PADDING - 1,  win_height-2, goback_text(obj));
        newtFormAddComponents(form, bCancel, textbox, bOk, NULL);
    } else {
        bOk     = newtCompactButton((win_width-strwidth(continue_text(obj))-2)/2 - 1, win_height-2, continue_text(obj));
        bCancel = NULL;
        newtFormAddComponents(form, textbox, bOk, NULL);
    }
    newtFormSetCurrent(form, bOk);
    cRet = newtRunForm(form);
    if (cRet == bOk)
        ret = DC_OK;
    else if ((cRet == NULL) || (bCancel != NULL && cRet == bCancel))
        ret = DC_GOBACK;
    else
        ret = DC_NOTOK;
    newtFormDestroy(form);
    newtPopWindow();
    free(descr);
    free(ext_descr);
    return ret;
}

static int
generic_handler_string(struct frontend *obj, struct question *q, int eflags)
{
    newtComponent form, textbox, bOk, bCancel, entry, cRet;
    int width = 80, height = 24, t_height, t_width, win_width, win_height;
    int t_width_scroll = 0, t_width_title, t_width_buttons;
    int ret;
#ifdef HAVE_LIBTEXTWRAP
    int tflags = 0;
    textwrap_t tw;
    char *wrappedtext;
#else
    int tflags = NEWT_FLAG_WRAP;
#endif
    char *defval;
    const char *result;
    char *full_description = get_full_description(q);

    eflags |= NEWT_ENTRY_SCROLL | NEWT_FLAG_RETURNEXIT;
    newtGetScreenSize(&width, &height);
    win_width = width-7;
    strtruncate(obj->title, win_width-9);
#ifdef HAVE_LIBTEXTWRAP
    textwrap_init(&tw);
    textwrap_columns(&tw, win_width - 2 - 2*TEXT_PADDING);
    wrappedtext = textwrap(&tw, full_description);
    free(full_description);
    full_description = wrappedtext;
#endif
    if (full_description != NULL)
        t_height = get_text_height(full_description, win_width);
    else
        t_height = 0;
    if (t_height + 6 <= height-5)
        win_height = t_height + 6;
    else {
        win_height = height - 5;
        tflags |= NEWT_FLAG_SCROLL;
        t_width_scroll = 2;
    }
    t_height = win_height - 6;
    t_width = get_text_width(full_description);
    t_width_buttons = 2*BUTTON_PADDING;
    if (obj->methods.can_go_back(obj, q))
        //  Add an interspace
        t_width_buttons += get_text_width(goback_text(obj)) + 3;
    if (t_width_buttons > t_width)
        t_width = t_width_buttons;
    if (win_width > t_width + 2*TEXT_PADDING + t_width_scroll)
        win_width = t_width + 2*TEXT_PADDING + t_width_scroll;
    t_width_title = get_text_width(obj->title);
    if (t_width_title > win_width)
        win_width = t_width_title;
    create_window(win_width, win_height, obj->title, q->priority);
    form = create_form(NULL);
    textbox = newtTextbox(TEXT_PADDING, 1, t_width, t_height, tflags);
    assert(textbox);
    if (full_description != NULL)
    {
        newtTextboxSetText(textbox, full_description);
        free(full_description);
    }
    if (eflags & NEWT_FLAG_PASSWORD || question_getvalue(q, "") == NULL)
        defval = "";
    else
        defval = (char *)question_getvalue(q, "");
    entry   = newtEntry(TEXT_PADDING, 1+t_height+1, defval, t_width, &result, eflags);
    if (obj->methods.can_go_back(obj, q)) {
        bOk     = newtCompactButton(win_width - TEXT_PADDING - BUTTON_PADDING - strwidth(continue_text(obj)) - 3, win_height-2, continue_text(obj));
        bCancel = newtCompactButton(TEXT_PADDING + BUTTON_PADDING - 1, win_height-2, goback_text(obj));
        newtFormAddComponents(form, bCancel, textbox, entry, bOk, NULL);
    } else {
        bOk     = newtCompactButton((win_width-strwidth(continue_text(obj))-2)/2 - 1, win_height-2, continue_text(obj));
        bCancel = NULL;
        newtFormAddComponents(form, textbox, entry, bOk, NULL);
    }
    newtFormSetCurrent(form, entry);
    cRet = newtRunForm(form);
    if ((cRet == NULL) || (bCancel != NULL && cRet == bCancel))
        ret = DC_GOBACK;
    else {
        ret = DC_OK;
        question_setvalue(q, result);
    }
    newtFormDestroy(form);
    newtPopWindow();
    return ret;
}

static int
show_multiselect_window(struct frontend *obj, struct question *q, int show_ext_desc)
{
    newtComponent form, sform = NULL, scrollbar, textbox, checkbox, bOk, bCancel, cRet;
    int width = 80, height = 24;
    int win_width, win_height = -1, t_height, t_width, sel_height, sel_width;
    int t_width_title, t_width_buttons;
    char **choices, **choices_trans, **defvals, *answer;
    int count = 0, defcount, i, k, ret, def;
    char *p;
    size_t res;
    wchar_t c;
    int *tindex = NULL;
    const char *indices = q_get_indices(q);
    char *full_description = get_full_description(q);
#ifdef HAVE_LIBTEXTWRAP
    textwrap_t tw;
    char *wrappedtext;
    int tflags = 0;
#else
    int tflags = NEWT_FLAG_WRAP;
#endif

    newtGetScreenSize(&width, &height);
    win_width = width-7;
    strtruncate(obj->title, win_width-9);
    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0)
        return DC_NOTOK;
    choices = malloc(sizeof(char *) * count);
    choices_trans = malloc(sizeof(char *) * count);
    tindex = malloc(sizeof(int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_trans, tindex, count) != count)
        return DC_NOTOK;

    defvals = malloc(sizeof(char *) * count);
    defcount = strchoicesplit(question_getvalue(q, ""), defvals, count);
    answer = malloc(sizeof(char) * count);
    sel_height = count;
    form = create_form(NULL);
#ifdef HAVE_LIBTEXTWRAP
    textwrap_init(&tw);
    textwrap_columns(&tw, win_width - 2 - 2*TEXT_PADDING);
    wrappedtext = textwrap(&tw, full_description);
    free(full_description);
    full_description = wrappedtext;
#endif
    sel_width = strlongest(choices_trans, count);
    t_width = get_text_width(full_description);
    t_width_buttons = 2*BUTTON_PADDING + get_text_width(continue_text(obj)) + 2;
    if (obj->methods.can_go_back(obj, q))
        //  Add an interspace
        t_width_buttons += get_text_width(goback_text(obj)) + 3;
    if (t_width_buttons > t_width)
        t_width = t_width_buttons;
    //  Truncate too wide items
    if (sel_width > win_width-8) {
        sel_width = win_width-8;
        for (i = 0; i < count; i++) {
            if (strwidth(choices_trans[i]) > sel_width) {
                for (res = 0, p = choices_trans[i]; (k = mbtowc (&c, p, MB_LEN_MAX)) > 0; p += k) {
                    res += wcwidth (c);
                    if (res > sel_width)
                        break;
                }
                *p = 0;
            }
        }
    }
    if (t_width < sel_width)
        t_width = sel_width;
    if (win_width > t_width + 8)
        win_width = t_width + 8;
    t_width_title = get_text_width(obj->title);
    if (t_width_title > win_width)
        win_width = t_width_title;
    if (show_ext_desc && full_description) {
        textbox = newtTextbox(TEXT_PADDING, 1, t_width, 10, tflags);
        assert(textbox);
        newtTextboxSetText(textbox, full_description);
        t_height = newtTextboxGetNumLines(textbox);
        newtTextboxSetHeight(textbox, t_height);
        newtFormAddComponent(form, textbox);
    } else
        t_height = 0;
    free(full_description);
    win_height = t_height + 5 + sel_height;
    if (win_height > height-5) {
        win_height = height-5;
        sel_height = win_height - t_height - 5;
    }
    create_window(win_width, win_height, obj->title, q->priority);
    if (count > sel_height) {
        scrollbar = newtVerticalScrollbar((win_width+sel_width+5)/2, 1+t_height+1, sel_height,
                NEWT_COLORSET_WINDOW, NEWT_COLORSET_ACTCHECKBOX);
        newtFormAddComponent(form, scrollbar);
    }
    else
        scrollbar = NULL;
    sform = create_form(scrollbar);
    newtFormSetBackground(sform, NEWT_COLORSET_CHECKBOX);
    newtFormSetHeight(sform, sel_height);
    newtFormSetWidth(sform, sel_width+5);
    for (i = 0; i < count; i++) {
        def = 0;
        for (k = 0; k < defcount; k++)
            if (strcmp(choices[tindex[i]], defvals[k]) == 0)
                def = 1;
        checkbox = newtCheckbox((win_width-sel_width-3)/2, 1+t_height+1+i, choices_trans[i], def ? '*' : ' ', " *", &answer[tindex[i]]);
        newtCheckboxSetFlags(checkbox, NEWT_FLAG_RETURNEXIT, NEWT_FLAGS_SET);
        newtFormAddComponent(sform, checkbox);
    }
    if (obj->methods.can_go_back(obj, q) || !show_ext_desc) {
        bOk     = newtCompactButton(win_width - TEXT_PADDING - BUTTON_PADDING - strwidth(continue_text(obj)) - 3, win_height-2, continue_text(obj));
        bCancel = newtCompactButton(TEXT_PADDING + BUTTON_PADDING - 1, win_height-2, goback_text(obj));
        newtFormAddComponents(form, bCancel, sform, bOk, NULL);
    } else {
        bOk     = newtCompactButton((win_width-strwidth(continue_text(obj))-2)/2 - 1, win_height-2, continue_text(obj));
        bCancel = NULL;
        newtFormAddComponents(form, sform, bOk, NULL);
    }
    newtFormSetCurrent(form, sform);
    cRet = newtRunForm(form);
    if ((cRet == NULL) || (bCancel != NULL && cRet == bCancel))
        ret = DC_GOBACK;
    else {
        char *ans = strdup(""), *tmp;

        for (i = 0; i < count; i++) {
            if (answer[i] != ' ') {
                if (ans[0] == '\0')
                    tmp = strdup(choices[i]);
                else
                    asprintf(&tmp, "%s, %s", ans, choices[i]);
                ans = tmp;
            }
            free(choices[i]);
            free(choices_trans[i]);
        }
        free(choices);
        free(choices_trans);
        free(tindex);
        free(answer);
        question_setvalue(q, ans);
        for (i = 0; i < defcount; i++)
            free(defvals[i]);
        free(defvals);
        ret = DC_OK;
    }
    newtFormDestroy(form);
    newtPopWindow();
    return ret;
}

static int
show_select_window(struct frontend *obj, struct question *q, int show_ext_desc)
{
    newtComponent form, listbox, textbox, bCancel, cRet;
    int listflags = NEWT_FLAG_RETURNEXIT;
    int width = 80, height = 24;
    int win_width, win_height = -1, t_height, t_width, sel_height, sel_width;
    int t_width_title, t_width_buttons;
    char **choices, **choices_trans, *defval;
    int count = 0, i, ret, defchoice = -1;
    int *tindex = NULL;
    const char *indices = q_get_indices(q);
    char *full_description = get_full_description(q);
    char *p;
    size_t res;
    int k;
    wchar_t c;
#ifdef HAVE_LIBTEXTWRAP
    textwrap_t tw;
    char *wrappedtext;
    int tflags = 0;
#else
    int tflags = NEWT_FLAG_WRAP;
#endif

    newtGetScreenSize(&width, &height);
    win_width = width-7;
    strtruncate(obj->title, win_width-9);
    count = strgetargc(q_get_choices_vals(q));
    if (count <= 0)
        return DC_NOTOK;
    choices = malloc(sizeof(char *) * count);
    choices_trans = malloc(sizeof(char *) * count);
    tindex = malloc(sizeof(int) * count);
    if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_trans, tindex, count) != count)
        return DC_NOTOK;

    sel_height = count;
    form = create_form(NULL);
#ifdef HAVE_LIBTEXTWRAP
    textwrap_init(&tw);
    textwrap_columns(&tw, win_width - 2 - 2*TEXT_PADDING);
    wrappedtext = textwrap(&tw, full_description);
    free(full_description);
    full_description = wrappedtext;
#endif
    sel_width = strlongest(choices_trans, count);
    t_width = get_text_width(full_description);
    t_width_buttons = 2*BUTTON_PADDING;
    if (obj->methods.can_go_back(obj, q))
        //  Add an interspace
        t_width_buttons += get_text_width(goback_text(obj)) + 3;
    if (t_width_buttons > t_width)
        t_width = t_width_buttons;
    //  Truncate too wide items
    if (sel_width > win_width-8) {
        sel_width = win_width-8;
        for (i = 0; i < count; i++) {
            if (strwidth(choices_trans[i]) > sel_width) {
                for (res = 0, p = choices_trans[i]; (k = mbtowc (&c, p, MB_LEN_MAX)) > 0; p += k) {
                    res += wcwidth (c);
                    if (res > sel_width)
                        break;
                }
                *p = 0;
            }
        }
    }
    if (t_width < sel_width)
        t_width = sel_width;
    if (win_width > t_width + 8)
        win_width = t_width + 8;
    t_width_title = get_text_width(obj->title);
    if (t_width_title > win_width)
        win_width = t_width_title;
    if (show_ext_desc && full_description) {
        textbox = newtTextbox(TEXT_PADDING, 1, t_width, 10, tflags);
        assert(textbox);
        newtTextboxSetText(textbox, full_description);
        t_height = newtTextboxGetNumLines(textbox);
        newtTextboxSetHeight(textbox, t_height);
        newtFormAddComponent(form, textbox);
    } else
        t_height = 0;
    free(full_description);
    win_height = t_height + 5 + sel_height;
    if (win_height > height-5) {
        win_height = height-5;
        sel_height = win_height - t_height - 5;
    }
    if (count > sel_height)
        listflags |= NEWT_FLAG_SCROLL;
    create_window(win_width, win_height, obj->title, q->priority);
    listbox = newtListbox((win_width-sel_width-3)/2, 1+t_height+1, sel_height, listflags);
    defval = (char *)question_getvalue(q, "");
    for (i = 0; i < count; i++) {
        newtListboxAppendEntry(listbox, choices_trans[i], choices[tindex[i]]);
        if (defval != NULL && strcmp(defval, choices[tindex[i]]) == 0)
            defchoice = i;
    }
    free(tindex);
    free(choices);
    free(choices_trans);
    if (count == 1)
        defchoice = 0;
    if (defchoice >= 0)
        newtListboxSetCurrent(listbox, defchoice);
    if (obj->methods.can_go_back(obj, q) || !show_ext_desc) {
        bCancel = newtCompactButton(TEXT_PADDING + BUTTON_PADDING - 1, win_height-2, goback_text(obj));
        newtFormAddComponents(form, bCancel, listbox, NULL);
    } else {
        bCancel = NULL;
        newtFormAddComponents(form, listbox, NULL);
    }
    newtFormSetCurrent(form, listbox);
    cRet = newtRunForm(form);
    if ((cRet == NULL) || (bCancel != NULL && cRet == bCancel))
        ret = DC_GOBACK;
    else {
        if (newtListboxGetCurrent(listbox) != NULL)
            question_setvalue(q, newtListboxGetCurrent(listbox));
        ret = DC_OK;
    }
    newtFormDestroy(form);
    newtPopWindow();
    return ret;
}

/*
 * Function: newt_handler_boolean
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the boolean question type
 * Assumptions: none
 */
static int
newt_handler_boolean(struct frontend *obj, struct question *q)
{
    newtComponent form, bYes, bNo, bCancel, textbox, cRet;
    int width = 80, height = 24;
    int win_width, win_height = -1, t_height, t_width;
    int t_width_scroll = 0, t_width_title, t_width_buttons;
    int ret;
#ifdef HAVE_LIBTEXTWRAP
    textwrap_t tw;
    char *wrappedtext;
    int flags = 0;
#else
    int flags = NEWT_FLAG_WRAP;
#endif
    char *full_description = get_full_description(q);

    newtGetScreenSize(&width, &height);
    win_width = width-7;
    strtruncate(obj->title, win_width-9);
#ifdef HAVE_LIBTEXTWRAP
    textwrap_init(&tw);
    textwrap_columns(&tw, win_width - 2 - 2*TEXT_PADDING);
    wrappedtext = textwrap(&tw, full_description);
    free(full_description);
    full_description = wrappedtext;
#endif
    if (full_description != NULL)
        t_height = get_text_height(full_description, win_width);
    else
        t_height = 0;
    if (t_height + 4 <= height-5)
        win_height = t_height + 4;
    else {
        win_height = height-5;
        flags |= NEWT_FLAG_SCROLL;
        t_width_scroll = 2;
    }
    t_height = win_height - 4;
    t_width = get_text_width(full_description);
    t_width_buttons = 2*BUTTON_PADDING;
    t_width_buttons += get_text_width(yes_text(obj)) + 3;
    t_width_buttons += get_text_width(no_text(obj)) + 3;
    if (obj->methods.can_go_back(obj, q))
        t_width_buttons += 3;
    if (obj->methods.can_go_back(obj, q))
        t_width_buttons += get_text_width(goback_text(obj)) + 3;
    if (t_width_buttons > t_width)
        t_width = t_width_buttons;
    if (win_width > t_width + 2*TEXT_PADDING + t_width_scroll)
        win_width = t_width + 2*TEXT_PADDING + t_width_scroll;
    t_width_title = get_text_width(obj->title);
    if (t_width_title > win_width)
        win_width = t_width_title;
    create_window(win_width, win_height, obj->title, q->priority);
    form = create_form(NULL);
    textbox = newtTextbox(TEXT_PADDING, 1, t_width, t_height, flags);
    assert(textbox);
    if (full_description != NULL)
        newtTextboxSetText(textbox, full_description);
    free(full_description);
    if (obj->methods.can_go_back(obj, q)) {
        bYes = newtCompactButton(win_width - TEXT_PADDING - BUTTON_PADDING - strwidth(yes_text(obj)) - 3 - strwidth(no_text(obj)) - 3 - 3, win_height-2, yes_text(obj));
        bNo = newtCompactButton(win_width - TEXT_PADDING - BUTTON_PADDING - strwidth(no_text(obj)) - 3, win_height-2, no_text(obj));
        bCancel = newtCompactButton(TEXT_PADDING + BUTTON_PADDING - 1, win_height-2, goback_text(obj));
        newtFormAddComponents(form, bCancel, textbox, bYes, bNo, NULL);
    } else {
        bYes = newtCompactButton(TEXT_PADDING + BUTTON_PADDING - 1, win_height-2, yes_text(obj));
        bNo = newtCompactButton(win_width - TEXT_PADDING - BUTTON_PADDING - strwidth(no_text(obj)) - 3, win_height-2, no_text(obj));
        bCancel = NULL;
        newtFormAddComponents(form, textbox, bYes, bNo, NULL);
    }
    if (question_getvalue(q, "") != NULL && strcmp(question_getvalue(q, ""), "true") == 0)
        newtFormSetCurrent(form, bYes);
    else
        newtFormSetCurrent(form, bNo);
    cRet = newtRunForm(form);
    if (cRet == bYes) {
        ret = DC_OK;
        question_setvalue(q, "true");
    } else if (cRet == bNo) {
        ret = DC_OK;
        question_setvalue(q, "false");
    } else if ((cRet == NULL) || (bCancel != NULL && cRet == bCancel))
        ret = DC_GOBACK;
    else
        ret = DC_NOTOK;
    newtFormDestroy(form);
    newtPopWindow();
    return ret;
}

/*
 * Function: newt_handler_multiselect
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the multiselect question type
 * Assumptions: none
 */
static int
newt_handler_multiselect(struct frontend *obj, struct question *q)
{
    int separate_window, ret;

    separate_window = need_separate_window(q);
    while (1) {
        if (separate_window) {
            ret = show_separate_window(obj, q);
            if (ret != DC_OK)
                return ret;
            ret = show_multiselect_window(obj, q, 0);
        }
        else
            ret = show_multiselect_window(obj, q, 1);
        if (!separate_window || ret != DC_GOBACK)
            return ret;
    }
}

/*
 * Function: newt_handler_select
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the select question type
 * Assumptions: none
 */
static int
newt_handler_select(struct frontend *obj, struct question *q)
{
    int separate_window, ret;

    separate_window = need_separate_window(q);
    while (1) {
        if (separate_window) {
            ret = show_separate_window(obj, q);
            if (ret != DC_OK)
                return ret;
            ret = show_select_window(obj, q, 0);
        }
        else
            ret = show_select_window(obj, q, 1);
        if (!separate_window || ret != DC_GOBACK)
            return ret;
    }
}

/*
 * Function: newt_handler_string
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the string question type
 * Assumptions: none
 */
static int
newt_handler_string(struct frontend *obj, struct question *q)
{
    return generic_handler_string(obj, q, 0);
}

/*
 * Function: newt_handler_password
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the password question type
 * Assumptions: none
 */
static int newt_handler_password(struct frontend *obj, struct question *q)
{
    return generic_handler_string(obj, q, NEWT_FLAG_PASSWORD);
}

/*
 * Function: newt_handler_note
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the note question type
 * Assumptions: none
 */
static int newt_handler_note(struct frontend *obj, struct question *q)
{
    // Sort of hack-ish, yes, but it works :-)
    return show_separate_window(obj, q);
}

/*
 * Function: newt_handler_text
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the text question type
 * Assumptions: none
 */
static int
newt_handler_text(struct frontend *obj, struct question *q)
{
    /* FIXME: Is text really an interactive question type? */
    return newt_handler_note(obj, q);
}

static int
newt_handler_error(struct frontend *obj, struct question *q)
{
    char *oldcolor;
    int ret;
    struct newtColors palette = newtDefaultColorPalette;

    oldcolor = palette.rootBg;
    palette.rootBg = "red";
    newtSetColors(palette);
    ret = newt_handler_note(obj, q);
    palette.rootBg = oldcolor;
    newtSetColors(palette);
    return ret;
}

/* ----------------------------------------------------------------------- */
struct question_handlers {
	const char *type;
	newt_handler *handler;
} question_handlers[] = {
	{ "boolean",	newt_handler_boolean },         // OK
	{ "multiselect", newt_handler_multiselect },
	{ "select",	newt_handler_select },          // OK
	{ "string",	newt_handler_string },          // OK
	{ "password",	newt_handler_password },        // OK
	{ "note",	newt_handler_note },            // OK
	{ "text",	newt_handler_text },
	{ "error",	newt_handler_error },
	{ "",		NULL },
};

/*
 * Function: newt_initialize
 * Input: struct frontend *obj - frontend UI object
 *        struct configuration *cfg - configuration parameters
 * Output: int - DC_OK, DC_NOTOK
 * Description: initializes the text UI
 * Assumptions: none
 *
 * TODO: SIGINT is ignored by the newt UI, otherwise it interferes with
 * the way question/answers are handled. this is probably not optimal
 */
static int
newt_initialize(struct frontend *obj, struct configuration *conf)
{
    int i, width = 80, height = 24;

    obj->interactive = 1;
    obj->data = calloc(1, sizeof(struct newt_data));
    SLang_init_tty(0, 1, 0); /* disable flow control */
    newtInit();
    newtGetScreenSize(&width, &height);
    // Fill the screen so people can shift-pgup properly
    for (i = 0; i < height; i++)
        putchar('\n');
    newtFinished();
    return DC_OK;
}


/*
 * Function: newt_go
 * Input: struct frontend *obj - frontend object
 * Output: int - DC_OK, DC_NOTOK
 * Description: asks all pending questions
 * Assumptions: none
 */
static int
newt_go(struct frontend *obj)
{
    struct newt_data *data = (struct newt_data *)obj->data;
    struct question *q = obj->questions;
    int i, ret = DC_OK, cleared;

    cleared = 0;
    while (q != NULL) {
        for (i = 0; i < DIM(question_handlers); i++) {
            newt_handler *handler;
            struct plugin *plugin = NULL;

            if (*question_handlers[i].type)
                handler = question_handlers[i].handler;
            else {
                plugin = plugin_find(obj, q->template->type);
                if (plugin) {
                    INFO(INFO_DEBUG, "Found plugin for %s", q->template->type);
                    handler = (newt_handler *) plugin->handler;
                } else {
                    INFO(INFO_DEBUG, "No plugin for %s", q->template->type);
                    continue;
                }
            }

            if (plugin || strcmp(q->template->type, question_handlers[i].type) == 0) {
                if (!cleared && !data->scale_form) {
                    cleared = 1;
                    SLang_init_tty(0, 1, 0); /* disable flow control */
                    newtInit();
                    newtCls();
                }
                if (obj->info != NULL) {
                    char *text = q_get_description(obj->info);
                    if (text)
                        newtDrawRootText(0, 0, text);
                    free(text);
                }
                newtPushHelpLine(help_text(obj));
                ret = handler(obj, q);
                newtPopHelpLine();
                if (ret == DC_OK)
                    obj->qdb->methods.set(obj->qdb, q);
                else if (ret == DC_GOBACK && q->prev != NULL)
                    q = q->prev;
                else {
                    if (cleared && !data->scale_form)
                        newtFinished();
                    if (plugin)
                        plugin_delete(plugin);
                    return ret;
                }
                if (plugin)
                    plugin_delete(plugin);
                break;
            }
        }
        if (ret == DC_OK)
            q = q->next;
    }
    if (cleared && !data->scale_form)
        newtFinished();
    return DC_OK;
}

static bool
newt_can_go_back(struct frontend *obj, struct question *q)
{
    return (obj->capability & DCF_CAPB_BACKUP);
}

static void
newt_progress_start(struct frontend *obj, int min, int max, const char *title)
{
    struct newt_data *data = (struct newt_data *)obj->data;
    int width = 80, win_width;
#ifdef HAVE_LIBTEXTWRAP
    int flags = 0;
#else
    int flags = NEWT_FLAG_WRAP;
#endif

    DELETE(obj->progress_title);
    obj->progress_title = strdup(title);
    obj->progress_min = min;
    obj->progress_max = max;
    obj->progress_cur = min;
    SLang_init_tty(0, 1, 0); /* disable flow control */
    newtInit();
    newtCls();
    if (obj->info != NULL) {
        char *text = q_get_description(obj->info);
        if (text)
            newtDrawRootText(0, 0, text);
        free(text);
    }
    newtGetScreenSize(&width, NULL);
    win_width = width-7;
    strtruncate(obj->progress_title, win_width-4);
    newtCenteredWindow(win_width, 5, obj->progress_title);
    data->scale_bar = newtScale(TEXT_PADDING, 1, win_width-2*TEXT_PADDING, obj->progress_max - obj->progress_min);
    /*  Minimal height set to 2 to prevent box flashing */
    data->scale_textbox = newtTextbox(TEXT_PADDING, 3, win_width-2*TEXT_PADDING, 2, flags);
    data->scale_textbox_height = 2;
    data->scale_form = create_form(NULL);
    newtFormAddComponents(data->scale_form, data->scale_bar, data->scale_textbox, NULL);
    newtDrawForm(data->scale_form);
    newtRefresh();
}

static void
newt_progress_set(struct frontend *obj, int val)
{
    struct newt_data *data = (struct newt_data *)obj->data;

    if (data->scale_form != NULL)
    {
	obj->progress_cur = val;
/*	if (obj->progress_max - obj->progress_min > 0)
	{
	    char buf[64];
	    float perc;

	    perc = 100.0 * (float)(obj->progress_cur - obj->progress_min) / 
		(float)(obj->progress_max - obj->progress_min);
	    sprintf(buf, "%3d%%", (int)perc);
	    newtLabelSetText(data->perc_label, buf);
	} */
	newtScaleSet(data->scale_bar, obj->progress_cur - obj->progress_min);
	newtDrawForm(data->scale_form);
	newtRefresh();
    }
}

static void
newt_progress_info(struct frontend *obj, const char *info)
{
    struct newt_data *data = (struct newt_data *)obj->data;

    if (data->scale_form != NULL) {
	int width, win_width, text_height;
#ifdef HAVE_LIBTEXTWRAP
	textwrap_t tw;
	char *wrappedtext;
	int flags = 0;
#else
	int flags = NEWT_FLAG_WRAP;
#endif

	newtGetScreenSize(&width, NULL);
	win_width = width-7;
	text_height = get_text_height(info, win_width);
	if (text_height < 2)
	    text_height = 2;
	if (text_height != data->scale_textbox_height) {
	    newtFormDestroy(data->scale_form);
	    newtPopWindow();
	    newtCenteredWindow(win_width, 4 + text_height, obj->progress_title);
	    data->scale_bar = newtScale(TEXT_PADDING, 1, win_width-2*TEXT_PADDING, obj->progress_max);
	    newtScaleSet(data->scale_bar, obj->progress_cur);
	    data->scale_textbox = newtTextbox(TEXT_PADDING, 3, win_width-2*TEXT_PADDING, text_height, flags);
	    data->scale_textbox_height = text_height;
	    data->scale_form = create_form(NULL);
	    newtFormAddComponents(data->scale_form, data->scale_bar, data->scale_textbox, NULL);
	}
#ifdef HAVE_LIBTEXTWRAP
	textwrap_init(&tw);
	textwrap_columns(&tw, win_width - 2 - 2*TEXT_PADDING);
	wrappedtext = textwrap(&tw, info);
	newtTextboxSetText(data->scale_textbox, wrappedtext);
	free(wrappedtext);
#else
	newtTextboxSetText(data->scale_textbox, info);
#endif
	newtDrawForm(data->scale_form);
	newtRefresh();
    }
}

static void
newt_progress_stop(struct frontend *obj)
{
    struct newt_data *data = (struct newt_data *)obj->data;

    if (data->scale_form != NULL) {
        newtFormDestroy(data->scale_form);
        newtPopWindow();
        newtFinished();
        data->scale_form = data->scale_bar = data->perc_label = data->scale_textbox = NULL;
    }
}

static int
newt_shutdown(struct frontend *obj)
{
    newt_progress_stop(obj);
    newtFinished();
    return DC_OK;
}

struct frontend_module debconf_frontend_module =
{
initialize: newt_initialize,
shutdown: newt_shutdown,
go: newt_go,
can_go_back: newt_can_go_back,
progress_start: newt_progress_start,
progress_set:   newt_progress_set,
progress_info:  newt_progress_info,
progress_stop:  newt_progress_stop,
};
