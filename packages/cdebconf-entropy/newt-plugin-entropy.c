/*
 * cdebconf newt plugin to get random data
 *
 * Copyright 2005-2006 by Max Vozeler <xam@debian.org>
 * See debian/copyright
 *
 * $Id$
 *
 */

#define _GNU_SOURCE

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <dlfcn.h>
#include <errno.h>
#include <newt.h>

#include <cdebconf/constants.h>
#include <cdebconf/frontend.h>
#include <cdebconf/question.h>
#include <cdebconf/template.h>
#include <cdebconf/cdebconf_newt.h>

#define HAVE_LIBTEXTWRAP
#ifdef HAVE_LIBTEXTWRAP
#include <textwrap.h>
#endif

#define MODULE "entropy"
#define FIFO "/var/run/random.fifo"
#define KEYSIZE 2925

#define error(fmt, args...) syslog(LOG_ERR, MODULE ": " fmt, ##args)

/* From cdebconf/strutl.c */
extern int strtruncate (char *what, size_t maxsize);
extern size_t strwidth (const char *what);

static void
prepare_window(newtComponent *, struct frontend *, struct question *, int);

static char rnd_byte;
static newtComponent entry;
static newtComponent textbox;
static newtComponent textbox2;
static newtComponent scale;
static newtComponent bCancel;
static newtComponent bOK;

static const char *
continue_text(struct frontend *obj)
{
    return question_get_text(obj, "debconf/button-continue", "Continue");
}

static const char *
goback_text(struct frontend *obj)
{
    return question_get_text(obj, "debconf/button-goback", "Go Back");
}

static const char *
help_text(struct frontend *obj)
{
    return question_get_text(obj, "debconf/help-line", "<Tab> moves between items; <Space> selects; <Enter> activates buttons");
}

static const char *
action_text(struct frontend *obj)
{
    return question_get_text(obj, "debconf/entropy/text/action", "Enter random characters");
}

static const char *
entropy_help_text(struct frontend *obj)
{
    return question_get_text(obj, "debconf/entropy/text/help",
        "You can help speed up the process by entering random characters on "
        "the keyboard, or just wait until enough key data has been collected "
        "(which can take a long time).");
}

static const char *
success_text(struct frontend *obj, struct question *q)
{
    const char *success;

    if (NULL == (success = question_get_variable(q, "SUCCESS"))) {
        success = "debconf/entropy/success";
    }
    return question_get_text(obj, success,
      "Key data has been created successfully.");
}

static int
copy_byte(int in, int out)
{
    ssize_t n;

    if ((n = read(in, &rnd_byte, 1)) < 1) {
        rnd_byte = 0;
        error("read: %s", (n == 0) ? "short read" : strerror(errno));
        return -1;
    }

    /* newtFormWatchFd for some reason doesn't detect when the
     * fifo is writable, so this write can block if there isn't
     * anyone reading from the fifo.
     */
    if ((n = write(out, &rnd_byte, 1)) < 1) {
        rnd_byte = 0;
        error("write: %s", (n == 0) ? "short write" : strerror(errno));
        return -1;
    }

    rnd_byte = 0;
    return 0;
}

int
cdebconf_newt_handler_entropy(struct frontend *obj, struct question *q)
{
    newtComponent form;
    struct newtExitStruct nstat;
    const char *p;
    int ret = DC_NOTOK;
    int keysize;
    const char *fifo = NULL;
    int nwritten = 0;
    int want_data = 1;
    int randfd = 0;
    int fifofd = 0;

    if (mlock(&rnd_byte, sizeof(rnd_byte)) < 0) {
        error("mlock failed: %s", strerror(errno));
        goto errout;
    }

    if (NULL == (fifo = question_get_variable(q, "FIFO"))) {
        fifo = FIFO;
    }

    if (mkfifo(fifo, 0600) < 0) {
        error("mkfifo(%s): %s", fifo, strerror(errno));
        goto errout;
    }

    randfd = open("/dev/random", O_RDONLY);
    if (randfd < 0) {
        error("open(/dev/random): %s",strerror(errno));
        goto errout;
    }

    fifofd = open(fifo, O_WRONLY);
    if (fifofd < 0) {
        error("open(%s): %s", fifo, strerror(errno));
        goto errout;
    }

    keysize = KEYSIZE;
    p = question_get_variable(q, "KEYSIZE");
    if (p)
        keysize = atoi(p);

    prepare_window(&form, obj, q, keysize);
    newtFormWatchFd(form, randfd, NEWT_FD_READ);

    while (1) {
        newtPushHelpLine(help_text(obj));
        newtFormRun(form, &nstat);
        newtPopHelpLine();

        /* Button pressed */
        if (nstat.reason == NEWT_EXIT_COMPONENT) {
            if (!nstat.u.co) {
                error("exit from unknown component");
                break;
            }

            if (nstat.u.co == bOK) {
                ret = DC_OK;
                break;
            }

            if (nstat.u.co == bCancel) {
                ret = DC_GOBACK;
                break;
            }
        }

        /* Data is ready */
        if (nstat.reason == NEWT_EXIT_FDREADY && want_data) {
            if (copy_byte(randfd, fifofd) < 0) {
                ret = DC_NOTOK;
                break;
            }

            nwritten++;
            newtScaleSet(scale, nwritten);
            newtEntrySet(entry, "", 0);

            if (nwritten == keysize) {
                /* Done - activate OK button */
                newtTextboxSetText(textbox, success_text(obj, q));
                newtComponentTakesFocus(bOK, 1);
                newtFormSetCurrent(form, bOK);
                want_data = 0;
            }
        }
    }

errout:
    if (randfd)
        close(randfd);
    if (fifofd)
        close(fifofd);

    if (NULL != fifo) {
        unlink(fifo);
    }

    munlock(&rnd_byte, sizeof(rnd_byte));
    return ret;
}

static void
prepare_window(newtComponent *form, struct frontend *obj, struct question *q, int keysize)
{
    int width = 80;
    int height = 24;
    int t_width, t_height;
    int t_width_buttons, t_width_title, t_width_scroll = 0;
    int win_width, win_height;
    char *description;
    const char *result;

#ifdef HAVE_LIBTEXTWRAP
    int tflags = 0;
    textwrap_t tw;
    char *wrappedtext;
#else
    int tflags = NEWT_FLAG_WRAP;
#endif
    int eflags = NEWT_ENTRY_SCROLL | NEWT_FLAG_RETURNEXIT | NEWT_FLAG_PASSWORD;

    newtGetScreenSize(&width, &height);
    win_width = width-7;
    /*  There are 5 characters for sigils, plus 4 for borders */
    strtruncate(obj->title, win_width-9);

    asprintf(&description, "%s\n\n%s",
        question_get_field(obj, q, "", "description"), entropy_help_text(obj));

#ifdef HAVE_LIBTEXTWRAP
    textwrap_init(&tw);
    textwrap_columns(&tw, win_width - 2 - 2*TEXT_PADDING);
    wrappedtext = textwrap(&tw, description);
    free(description);
    description = wrappedtext;
#endif

    t_height = cdebconf_newt_get_text_height(description, win_width);
    if (t_height + 6 + 4 <= height-5)
        win_height = t_height + 6 + 4;
    else {
        win_height = height - 5;
        tflags |= NEWT_FLAG_SCROLL;
        t_width_scroll = 2;
    }

    t_height = win_height - (6 + 4);
    t_width = cdebconf_newt_get_text_width(description);
    t_width_buttons = 2*BUTTON_PADDING;
    t_width_buttons += cdebconf_newt_get_text_width(goback_text(obj)) + 3;
    t_width_buttons += cdebconf_newt_get_text_width(continue_text(obj)) + 3;

    if (t_width_buttons > t_width)
        t_width = t_width_buttons;

    if (win_width > t_width + 2*TEXT_PADDING + t_width_scroll)
        win_width = t_width + 2*TEXT_PADDING + t_width_scroll;

    t_width_title = cdebconf_newt_get_text_width(obj->title);
    if (t_width_title > win_width)
        win_width = t_width_title;

    cdebconf_newt_create_window(win_width, win_height, obj->title, q->priority);
    *form = newtForm(NULL, NULL, 0);

    textbox = newtTextbox(TEXT_PADDING, 1, t_width, t_height, tflags);
    scale = newtScale(TEXT_PADDING, 1+t_height+1, win_width-2*TEXT_PADDING, keysize);
    textbox2 = newtTextbox(TEXT_PADDING, 1+t_height+3, t_width, 1, tflags);
    entry = newtEntry(TEXT_PADDING, 1+t_height+5, "", t_width, &result, eflags);

    newtFormAddComponents(*form, scale, textbox, textbox2, entry, NULL);
    if (obj->methods.can_go_back(obj, q)) {
        bCancel = newtCompactButton((TEXT_PADDING + BUTTON_PADDING - 1), win_height-2, goback_text(obj));
        newtFormAddComponents(*form, bCancel, NULL);
    } else {
        bCancel = NULL;
    }

    bOK = newtCompactButton((win_width - TEXT_PADDING - BUTTON_PADDING - strwidth(continue_text(obj))-3), win_height-2, continue_text(obj));
    newtComponentTakesFocus(bOK, 0);
    newtFormAddComponent(*form, bOK);

    newtScaleSet(scale, 0);
    newtTextboxSetText(textbox, description);
    newtTextboxSetText(textbox2, action_text(obj));
    free(description);
}

/* vim:set ts=4 sw=4 expandtab: */
