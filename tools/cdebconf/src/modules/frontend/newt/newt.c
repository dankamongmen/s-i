/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: newt.c
 *
 * Description: Newt UI for cdebconf
 *
 * $Id: newt.c,v 1.12 2003/05/11 14:00:51 sjogren Exp $
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
#include <assert.h>

#include <newt.h>

struct newt_data {
    newtComponent scale_form,
                  scale_bar,
                  scale_label,
                  perc_label;
};

#define q_get_extended_description(q)   question_get_field((q), "", "extended_description")
#define q_get_description(q)  		question_get_field((q), "", "description")
#define q_get_choices(q)		question_get_field((q), "", "choices")
#define q_get_choices_vals(q)		question_get_field((q), NULL, "choices")

#define create_form(scrollbar)          newtForm((scrollbar), NULL, 0)

/* To be i18ned */
static char *
continue_text(void)
{
    return "Continue";
}

/* To be i18ned */
static char *
goback_text(void)
{
    return "Go Back";
}

/* To be i18ned */
static char *
yes_text(void)
{
    return "Yes";
}

/* To be i18ned */
static char *
no_text(void)
{
    return "No";
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

    // Create a dummy testbox to find out how many lines the text in the
    // question will occupy
    textbox = newtTextbox(1, 2, win_width - 4, 10, NEWT_FLAG_SCROLL|NEWT_FLAG_WRAP);
    newtTextboxSetText(textbox, text);
    t_height = newtTextboxGetNumLines(textbox);
    // This is needed so the textbox gets freed...ick
    f = create_form(NULL);
    newtFormAddComponent(f, textbox);
    newtFormDestroy(f);
    return t_height;
}

static int
min_window_height(struct question *q, int win_width)
{
    int height;
    char *type = q->template->type;

    height = get_text_height(q_get_extended_description(q), win_width) + 4;
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

    newtGetScreenSize(&width, &height);
    return (min_window_height(q, width-7) >= height-5);
}

static int
show_separate_window(struct frontend *obj, struct question *q)
{
    newtComponent form, textbox, bOk, bCancel, cRet;
    int width = 80, height = 24, t_height, win_width, win_height;
    // title + buttons
    int extra = 1 + 3;
    int flags = NEWT_FLAG_WRAP, ret;

    newtGetScreenSize(&width, &height);
    win_width = width-7;
    t_height = get_text_height(q_get_extended_description(q), win_width);
    if (t_height+extra <= height-5)
        win_height = t_height+extra;
    else {
        win_height = height-5;
        flags |= NEWT_FLAG_SCROLL;
    }
    t_height = win_height-extra;
    create_window(width-7, win_height, obj->title, q->priority);
    form = create_form(NULL);
    newtFormAddComponent(form, newtLabel((win_width - strwidth(q_get_description(q)))/2, 0, q_get_description(q)));
    textbox = newtTextbox(1, 1, win_width-4, t_height, flags);
    newtTextboxSetText(textbox, q_get_extended_description(q));
    bOk     = newtCompactButton(5, win_height-2, continue_text());
    bCancel = newtCompactButton(win_width - 9 - strwidth(goback_text()), win_height-2, goback_text());
    newtComponentTakesFocus(bCancel, obj->methods.can_go_back(obj, q));
    newtFormAddComponents(form, textbox, bOk, bCancel, NULL);
    cRet = newtRunForm(form);
    if (cRet == bOk)
        ret = DC_OK;
    else if (cRet == bCancel)
        ret = DC_GOBACK;
    else
        ret = DC_NOTOK;
    newtFormDestroy(form);
    newtPopWindow();
    return ret;
}

static int
generic_handler_string(struct frontend *obj, struct question *q, int eflags)
{
    newtComponent form, textbox, bOk, bCancel, entry, cRet;
    int width = 80, height = 24, t_height, win_width, win_height;
    int ret, tflags = NEWT_FLAG_WRAP;
    char *defval, *result;

    eflags |= NEWT_ENTRY_SCROLL | NEWT_FLAG_RETURNEXIT;
    newtGetScreenSize(&width, &height);
    win_width = width-7;
    t_height = get_text_height(q_get_extended_description(q), win_width);
    if (t_height + 6 <= height-5)
        win_height = t_height + 6;
    else {
        win_height = height - 5;
        tflags |= NEWT_FLAG_SCROLL;
    }
    t_height = win_height - 6;
    create_window(width-7, win_height, obj->title, q->priority);
    form = create_form(NULL);
    newtFormAddComponent(form, newtLabel((win_width - strwidth(q_get_description(q)))/2, 0, q_get_description(q)));
    textbox = newtTextbox(1, 1, win_width-4, t_height, tflags);
    newtTextboxSetText(textbox, q_get_extended_description(q));
    if (eflags & NEWT_FLAG_HIDDEN || question_getvalue(q, "") == NULL)
        defval = "";
    else
        defval = (char *)question_getvalue(q, "");
    entry   = newtEntry(1, 1+t_height+1, defval, win_width-4, &result, eflags);
    bOk     = newtCompactButton(5, win_height-2, continue_text());
    bCancel = newtCompactButton(win_width - 9 - strwidth(goback_text()), win_height-2, goback_text());
    newtComponentTakesFocus(bCancel, obj->methods.can_go_back(obj, q));
    newtFormAddComponents(form, textbox, bOk, bCancel, entry, NULL);
    newtFormSetCurrent(form, entry);
    cRet = newtRunForm(form);
    if (cRet == bCancel)
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
    newtComponent form, sform = NULL, scrollbar, textbox, label, bOk, bCancel, cRet;
    int width = 80, height = 24;
    int win_width, win_height = -1, t_height, sel_height, sel_width;
    char *choices[100], *choices_trans[100], *defvals[100], answer[100];
    int count, defcount, i, k, ret, def;

    newtGetScreenSize(&width, &height);
    count = strchoicesplit(q_get_choices_vals(q), choices, 100);
    if (count <= 0)
        return DC_NOTOK;
    if (strchoicesplit(q_get_choices(q), choices_trans, 100) != count)
        return DC_NOTOK;
    defcount = strchoicesplit(question_getvalue(q, ""), defvals, 100);
    win_width = width-7;
    sel_height = count;
    form = create_form(NULL);
    if (show_ext_desc) {
        textbox = newtTextbox(1, 1, win_width-4, 10, NEWT_FLAG_WRAP);
        newtTextboxSetText(textbox, q_get_extended_description(q));
        t_height = newtTextboxGetNumLines(textbox);
        newtTextboxSetHeight(textbox, t_height);
        newtFormAddComponent(form, textbox);
    } else
        t_height = 0;
    win_height = t_height + 5 + sel_height;
    if (win_height > height-5) {
        win_height = height-5;
        sel_height = win_height - t_height - 5;
    }
    sel_width = 0;
    for (i = 0; i < count; i++) {
        if (strwidth(choices_trans[i]) > win_width-10) {
            sel_width = win_width-10;
            choices_trans[i][sel_width] = 0; // How do we handle wide characters here?
        } else if (strwidth(choices_trans[i]) > sel_width) {
            sel_width = strwidth(choices_trans[i]);
        }
    }
    create_window(win_width, win_height, obj->title, q->priority);
    label   = newtLabel((win_width - strwidth(q_get_description(q)))/2, 0, q_get_description(q));
    bOk     = newtCompactButton(5, win_height-2, continue_text());
    bCancel = newtCompactButton(win_width - 9 - strwidth(goback_text()), win_height-2, goback_text());
    newtComponentTakesFocus(bCancel, obj->methods.can_go_back(obj, q) || !show_ext_desc);
    newtFormAddComponents(form, label, bOk, bCancel, NULL);
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
            if (strcmp(choices[i], defvals[k]) == 0)
                def = 1;
        newtFormAddComponent(sform, newtCheckbox((win_width-sel_width-5)/2, 1+t_height+1+i, choices_trans[i], def ? '*' : ' ', " *", &answer[i]));
    }
    newtFormAddComponent(form, sform);
    newtFormSetCurrent(form, sform);
    cRet = newtRunForm(form);
    if (cRet == bCancel)
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
        question_setvalue(q, ans);
        for (i = 0; i < defcount; i++)
            free(defvals[i]);
        ret = DC_OK;
    }
    newtFormDestroy(form);
    newtPopWindow();
    return ret;
}

static int
show_select_window(struct frontend *obj, struct question *q, int show_ext_desc)
{
    newtComponent form, listbox, textbox, label, bOk, bCancel, cRet;
    int listflags = NEWT_FLAG_RETURNEXIT;
    int width = 80, height = 24;
    int win_width, win_height = -1, t_height, sel_height, sel_width;
    char *choices[100] = {NULL}, *choices_trans[100] = {NULL}, *defval;
    int count = 0, i, ret, defchoice = -1;

    newtGetScreenSize(&width, &height);
    count = strchoicesplit(q_get_choices_vals(q), choices, 100);
    if (count <= 0)
        return DC_NOTOK;
    if (strchoicesplit(q_get_choices(q), choices_trans, 100) != count)
        return DC_NOTOK;
    win_width = width-7;
    sel_height = count;
    form = create_form(NULL);
    if (show_ext_desc) {
        textbox = newtTextbox(1, 1, win_width-4, 10, NEWT_FLAG_WRAP);
        newtTextboxSetText(textbox, q_get_extended_description(q));
        t_height = newtTextboxGetNumLines(textbox);
        newtTextboxSetHeight(textbox, t_height);
        newtFormAddComponent(form, textbox);
    } else
        t_height = 0;
    win_height = t_height + 5 + sel_height;
    if (win_height > height-5) {
        win_height = height-5;
        sel_height = win_height - t_height - 5;
    }
    if (count > sel_height)
        listflags |= NEWT_FLAG_SCROLL;
    sel_width = 0;
    for (i = 0; i < count; i++) {
        if (strwidth(choices_trans[i]) > win_width-10) {
            sel_width = win_width-10;
            choices_trans[i][sel_width] = 0; // How do we handle wide characters here?
        } else if (strwidth(choices_trans[i]) > sel_width) {
            sel_width = strwidth(choices_trans[i]);
        }
    }
    create_window(win_width, win_height, obj->title, q->priority);
    label   = newtLabel((win_width - strwidth(q_get_description(q)))/2, 0, q_get_description(q));
    listbox = newtListbox((win_width - sel_width)/2, 1+t_height+1, sel_height, listflags);
    bOk     = newtCompactButton(5, win_height-2, continue_text());
    bCancel = newtCompactButton(win_width - 9 - strwidth(goback_text()), win_height-2, goback_text());
    defval = (char *)question_getvalue(q, "");
    for (i = 0; i < count; i++) {
        newtListboxAppendEntry(listbox, choices_trans[i], choices[i]);
        if (defval != NULL && strcmp(defval, choices[i]) == 0)
            defchoice = i;
    }
    if (count == 1)
        defchoice = 0;
    if (defchoice >= 0)
        newtListboxSetCurrent(listbox, defchoice);
    newtComponentTakesFocus(bCancel, obj->methods.can_go_back(obj, q) || !show_ext_desc);
    newtFormAddComponents(form, label, listbox, bOk, bCancel, NULL);
    cRet = newtRunForm(form);
    if (cRet == bCancel)
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
    int win_width, win_height = -1, t_height;
    int flags = NEWT_FLAG_WRAP, ret;

    newtGetScreenSize(&width, &height);
    win_width = width-7;
    t_height = get_text_height(q_get_extended_description(q), win_width);
    if (t_height + 4 <= height-5)
        win_height = t_height + 4;
    else {
        win_height = height-5;
        flags |= NEWT_FLAG_SCROLL;
    }
    t_height = win_height - 4;
    create_window(width-7, win_height, obj->title, q->priority);
    form = create_form(NULL);
    newtFormAddComponent(form, newtLabel((win_width - strwidth(q_get_description(q)))/2, 0, q_get_description(q)));
    textbox = newtTextbox(1, 1, win_width-4, t_height, flags);
    newtTextboxSetText(textbox, q_get_extended_description(q));
    bYes    = newtCompactButton(5, win_height-2, yes_text());
    bNo     = newtCompactButton((win_width - strwidth(no_text()) - 2)/2, win_height-2, no_text());
    bCancel = newtCompactButton(win_width - 9 - strwidth(goback_text()), win_height-2, goback_text());
    newtComponentTakesFocus(bCancel, obj->methods.can_go_back(obj, q));
    newtFormAddComponents(form, textbox, bYes, bNo, bCancel, NULL);
    if (strcmp(question_getvalue(q, ""), "true") == 0)
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
    } else if (cRet == bCancel)
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
    return generic_handler_string(obj, q, NEWT_FLAG_HIDDEN);
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
    /* FIXME: Is really text an interactive question type? */
    return newt_handler_note(obj, q);
}

/* ----------------------------------------------------------------------- */
struct question_handlers {
	const char *type;
	int (*handler)(struct frontend *obj, struct question *q);
} question_handlers[] = {
	{ "boolean",	newt_handler_boolean },         // OK
	{ "multiselect", newt_handler_multiselect },
	{ "select",	newt_handler_select },          // OK
	{ "string",	newt_handler_string },          // OK
	{ "password",	newt_handler_password },        // OK
	{ "note",	newt_handler_note },            // OK
	{ "text",	newt_handler_text }
};

/*
 * Function: newt_intitialize
 * Input: struct frontend *obj - frontend UI object
 *        struct configuration *cfg - configuration parameters
 * Output: int - DC_OK, DC_NOTOK
 * Description: initializes the text UI
 * Assumptions: none
 *
 * TODO: SIGINT is ignored by the newt UI, otherwise it interfers with
 * the way question/answers are handled. this is probably not optimal
 */
static int
newt_initialize(struct frontend *obj, struct configuration *conf)
{
    int i, width = 80, height = 24;

    obj->interactive = 1;
    obj->data = calloc(1, sizeof(struct newt_data));
    newtInit();
    newtGetScreenSize(&width, &height);
    // Fill the screen so people can shift-pgup properly
    for (i = 0; i < height; i++)
        putchar('\n');
    newtFinished();
    return DC_OK;
}

static int
newt_shutdown(struct frontend *obj)
{
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
    struct question *q = obj->questions;
    int i, ret = DC_OK, cleared;

    cleared = 0;
    while (q != NULL) {
        for (i = 0; i < DIM(question_handlers); i++)
            if (strcmp(q->template->type, question_handlers[i].type) == 0) {
                if (!cleared) {
                    cleared = 1;
                    newtInit();
                    newtCls();
                }
                ret = question_handlers[i].handler(obj, q);
                if (ret == DC_OK)
                    obj->qdb->methods.set(obj->qdb, q);
                else if (ret == DC_GOBACK && q->prev != NULL)
                    q = q->prev;
                else {
                    newtFinished();
                    return ret;
                }
                break;
            }
        if (ret == DC_OK)
            q = q->next;
    }
    if (cleared)
        newtFinished();
    return DC_OK;
}

static int
newt_can_go_back(struct frontend *obj, struct question *q)
{
    return (obj->capability & DCF_CAPB_BACKUP);
}

static void
newt_progress_start(struct frontend *obj, int min, int max, const char *title)
{
    struct newt_data *data = (struct newt_data *)obj->data;
    int width = 80, win_width;
    char buf[64];

    DELETE(obj->progress_title);
    obj->progress_title = NULL;
    obj->progress_min = min;
    obj->progress_max = max;
    obj->progress_cur = min;
    newtInit();
    newtCls();
    newtGetScreenSize(&width, NULL);
    win_width = width-7;
    newtCenteredWindow(win_width, 5, title);
    sprintf(buf, "%3d%%", 0);
    data->perc_label = newtLabel(1, 1, buf);
    data->scale_bar = newtScale(1 + strlen(buf) + 1, 1, win_width-strlen(buf)-3,
            obj->progress_max);
    data->scale_label = newtLabel(1, 3, "");
    data->scale_form = create_form(NULL);
    newtFormAddComponents(data->scale_form, data->perc_label, data->scale_bar,
            data->scale_label, NULL);
    newtDrawForm(data->scale_form);
    newtRefresh();
}

static void
newt_progress_set(struct frontend *obj, int val)
{
    struct newt_data *data = (struct newt_data *)obj->data;

    obj->progress_cur = val;
    if (obj->progress_max - obj->progress_min > 0)
    {
        char buf[64];
        float perc;

        perc = 100.0 * (float)(obj->progress_cur - obj->progress_min) / 
                       (float)(obj->progress_max - obj->progress_min);
        sprintf(buf, "%3d%%", (int)perc);
        newtLabelSetText(data->perc_label, buf);
    }
    newtScaleSet(data->scale_bar, obj->progress_cur);
    newtDrawForm(data->scale_form);
    newtRefresh();
}

static void
newt_progress_info(struct frontend *obj, const char *info)
{
    struct newt_data *data = (struct newt_data *)obj->data;

    newtLabelSetText(data->scale_label, info);
    newtDrawForm(data->scale_form);
    newtRefresh();
}

static void
newt_progress_stop(struct frontend *obj)
{
    struct newt_data *data = (struct newt_data *)obj->data;

    if (data->scale_form != NULL) {
        newtFormDestroy(data->scale_form);
        newtPopWindow();
        newtFinished();
        data->scale_form = data->scale_bar = data->perc_label = data->scale_label = NULL;
    }
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
