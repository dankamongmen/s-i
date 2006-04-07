/*
 * cdebconf newt plugin to get random data
 *
 * Copyright 2005-2006 by Max Vozeler <xam@debian.org>
 * See debian/copyright
 *
 * $Id$
 * 
 */ 

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
#include <cdebconf/config-newt.h>

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

/* There are dlopen()ed from frontend/newt.so */
int (*get_text_height)(const char *text, int win_width);
int (*get_text_width)(const char *text);
void(*create_window)(const int width, const int height, const char *title, const char *priority);

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
success_text(struct frontend *obj)
{
    return question_get_text(obj, "partman-crypto/entropy-text-success", 
      "Key data has been created successfully.");
}

static void *
setup_handler_dlsyms(void)
{
    char *err;
    void *newt;
    newt = dlopen("/usr/lib/cdebconf/frontend/newt.so", RTLD_LAZY);
    if (!newt) {
        error("dlopen on newt.so failed: %s", dlerror());
        return NULL;
    }

    dlerror();
    create_window = dlsym(newt, "newt_create_window");
    get_text_height = dlsym(newt, "newt_get_text_height");
    get_text_width = dlsym(newt, "newt_get_text_width");

    if ((err = dlerror()) != NULL)  {
        error("dlsym failed: %s", err);
        dlclose(newt);
        return NULL;
    }

    return newt;
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
newt_handler_entropy_text(struct frontend *obj, struct question *q)
{
    void *newt;
    newtComponent form;
    struct newtExitStruct nstat;
    const char *p;
    int ret = DC_NOTOK;
    int keysize;
    int nwritten = 0;
    int want_data = 1;
    int randfd = -1;
    int fifofd = -1;
 
    newt = setup_handler_dlsyms();
    if (!newt)
        return DC_NOTOK;
    
    if (mlock(&rnd_byte, sizeof(rnd_byte)) < 0) {
        error("mlock failed: %s", strerror(errno));
        goto errout;
    }
        
    if (mkfifo(FIFO, 0600) < 0) {
        error("mkfifo(%s): %s", FIFO, strerror(errno));
        goto errout;
    }

    randfd = open("/dev/random", O_RDONLY);
    if (randfd < 0) {
        error("open(/dev/random): %s",strerror(errno));
        goto errout;
    }

    fifofd = open(FIFO, O_WRONLY);
    if (fifofd < 0) {
        error("open(%s): %s", FIFO, strerror(errno));
        goto errout;
    }
   
    keysize = KEYSIZE;
    p = question_getvalue(q, "partman-crypto/keysize");
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
                newtTextboxSetText(textbox, success_text(obj));
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
    
    unlink(FIFO);
    dlclose(newt);

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
    char *ext_description;
    char *short_description;
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

    ext_description = question_get_field((q), "", "extended_description");
    short_description = question_get_field(q, "", "description");

#ifdef HAVE_LIBTEXTWRAP
    textwrap_init(&tw);
    textwrap_columns(&tw, win_width - 2 - 2*TEXT_PADDING);
    wrappedtext = textwrap(&tw, ext_description);
    free(ext_description);
    ext_description = wrappedtext;
#endif

    t_height = get_text_height(ext_description, win_width);
    if (t_height + 6 + 4 <= height-5)
        win_height = t_height + 6 + 4;
    else {
        win_height = height - 5;
        tflags |= NEWT_FLAG_SCROLL;
        t_width_scroll = 2;
    }

    t_height = win_height - (6 + 4);
    t_width = get_text_width(ext_description);
    t_width_buttons = 2*BUTTON_PADDING;
    t_width_buttons += get_text_width(goback_text(obj)) + 3;
    t_width_buttons += get_text_width(continue_text(obj)) + 3;

    if (t_width_buttons > t_width)
        t_width = t_width_buttons;
    
    if (win_width > t_width + 2*TEXT_PADDING + t_width_scroll)
        win_width = t_width + 2*TEXT_PADDING + t_width_scroll;

    t_width_title = get_text_width(obj->title);
    if (t_width_title > win_width)
        win_width = t_width_title;

    create_window(win_width, win_height, obj->title, q->priority);
    *form = newtForm(NULL, NULL, 0);
    
    textbox = newtTextbox(TEXT_PADDING, 1, t_width, t_height, tflags);
    scale = newtScale(TEXT_PADDING, 1+t_height+1, win_width-2*TEXT_PADDING, keysize);
    textbox2 = newtTextbox(TEXT_PADDING, 1+t_height+3, t_width, 1, tflags);
    entry = newtEntry(TEXT_PADDING, 1+t_height+5, "", t_width, &result, eflags);
    bCancel = newtCompactButton((TEXT_PADDING + BUTTON_PADDING - 1), win_height-2, goback_text(obj));

    newtFormAddComponents(*form, scale, textbox, textbox2, entry, bCancel, NULL);

    bOK = newtCompactButton((win_width - TEXT_PADDING - BUTTON_PADDING - strwidth(continue_text(obj))-3), win_height-2, continue_text(obj));
    newtComponentTakesFocus(bOK, 0);
    newtFormAddComponent(*form, bOK);

    newtScaleSet(scale, 0);
    newtTextboxSetText(textbox, ext_description);
    newtTextboxSetText(textbox2, short_description);
    free(ext_description);
    free(short_description);
}

/* vim:set ts=4 sw=4 expandtab: */
