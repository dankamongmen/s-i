/*
 * cdebconf newt plugin to get random data
 *
 * Copyright © 2008 by Jérémy Bobbio <lunar@debian.org>
 * See debian/copyright
 *
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <syslog.h>
#include <errno.h>
#include <termios.h>

#include <cdebconf/frontend.h>
#include <cdebconf/question.h>
#include <cdebconf/strutl.h>

#define MODULE "entropy"
#define FIFO "/var/run/random.fifo"
#define DEFAULT_KEYSIZE 2925

/* XXX: both should be exported by text frontend. */
#define error(fmt, args...) syslog(LOG_ERR, MODULE ": " fmt, ##args)
#define CHAR_GOBACK '<'

int cdebconf_text_handler_entropy(struct frontend * fe,
                                  struct question * question);

struct entropy {
    struct frontend * fe;
    struct question * question;
    long keysize;
    long bytes_read;
    int last_progress;
    const char * fifo;
    const char * success_template;
    int random_fd;
    int fifo_fd;
    char random_byte;
    int is_backing_up;
};

static void destroy_entropy(struct entropy * entropy_data)
{
    if (0 < entropy_data->fifo_fd) {
        (void) close(entropy_data->fifo_fd);
    }
    if (NULL != entropy_data->fifo) {
        (void) unlink(entropy_data->fifo);
    }
    if (0 < entropy_data->random_fd) {
        (void) close(entropy_data->random_fd);
    }
    (void) munlock(&entropy_data->random_byte, sizeof (char));
    free(entropy_data);
}

static int move_bytes(struct entropy * entropy_data)
{
    size_t n;

    while (entropy_data->bytes_read < entropy_data->keysize) {
        n = read(entropy_data->random_fd, &entropy_data->random_byte,
                 sizeof (char));
        if (sizeof (char) != n) {
            if (EAGAIN == errno) {
                break;
            }
            error("read failed: %s", strerror(errno));
            return DC_NOTOK;
        }
        n = write(entropy_data->fifo_fd, &entropy_data->random_byte,
                  sizeof (char));
        if (sizeof (char) != n) {
            error("write failed: %s", strerror(errno));
            return DC_NOTOK;
        }
        entropy_data->random_byte = 0;
        entropy_data->bytes_read++;
    }
    return DC_OK;
}

static int set_keysize(struct entropy * entropy_data,
                       struct question * question) {
    const char * keysize_string;

    keysize_string = question_get_variable(question, "KEYSIZE");
    if (NULL == keysize_string) {
        entropy_data->keysize = DEFAULT_KEYSIZE;
        return DC_OK;
    }
    entropy_data->keysize = strtol(keysize_string, NULL, 10);
    if (0 >= entropy_data->keysize || LONG_MAX == entropy_data->keysize) {
        error("keysize out of range");
        return DC_NOTOK;
    }
    return DC_OK;
}

static struct entropy * init_entropy(struct frontend * fe,
                                     struct question * question)
{
    struct entropy * entropy_data;

    if (NULL == (entropy_data = malloc(sizeof (struct entropy)))) {
        error("malloc failed.");
        return NULL;
    }
    memset(entropy_data, 0, sizeof (struct entropy));
    entropy_data->fe = fe;
    entropy_data->question = question;
    entropy_data->last_progress = -1;
    if (-1 == mlock(&entropy_data->random_byte, sizeof (char))) {
        error("mlock failed: %s", strerror(errno));
        goto failed;
    }
    entropy_data->success_template = question_get_variable(
        question, "SUCCESS");
    if (NULL == entropy_data->success_template) {
        entropy_data->success_template = "debconf/entropy/success";
    }
    entropy_data->random_fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
    if (-1 == entropy_data->random_fd) {
        error("open random_fd failed: %s", strerror(errno));
        goto failed;
    }
    entropy_data->fifo = question_get_variable(question, "FIFO");
    if (NULL == entropy_data->fifo) {
        entropy_data->fifo = FIFO;
    }
    if (-1 == mkfifo(entropy_data->fifo, 0600)) {
        error("mkfifo failed: %s", strerror(errno));
        goto failed;
    }
    entropy_data->fifo_fd = open(entropy_data->fifo, O_WRONLY);
    if (-1 == entropy_data->fifo_fd) {
        error("open fifo_fd failed: %s", strerror(errno));
        goto failed;
    }
    return entropy_data;

failed:
    destroy_entropy(entropy_data);
    return NULL;

}

/* XXX: Should be exported by text frontend. */
static int getwidth(void)
{
    static int res = 80;
    static int inited = DC_NOTOK;
    int fd;
    struct winsize ws;

    if (inited == DC_NOTOK) {
        inited = DC_OK;
        if (0 < (fd = open("/dev/tty", O_RDONLY))) {
            if (0 == ioctl(fd, TIOCGWINSZ, &ws) && 0 < ws.ws_col)
                res = ws.ws_col;
            close(fd);
        }
    }
    return res;
}

/* XXX: Should be exported by text frontend. */
static void wrap_print(const char *str)
{
    int i;
    int line_count;
    char * lines[500];

    line_count = strwrap(str, getwidth() - 1, lines, 499);

    for (i = 0; i < line_count; i++) {
        printf("%s\n", lines[i]);
        free(lines[i]);
    }
}

static void print_action(struct entropy * entropy_data)
{
    printf("%s: ",
        question_get_text(entropy_data->fe, "debconf/entropy/text/action",
                          "Enter random characters"));
}

static void print_help(struct entropy * entropy_data)
{
    wrap_print(question_get_text(
        entropy_data->fe, "debconf/entropy/text/help",
        "You can help speed up the process by entering random characters on "
        "the keyboard, or just wait until enough key data has been collected. "
        "(which can take a long time)."));
    printf("\n");
}

static void print_success(struct entropy * entropy_data)
{
    wrap_print(question_get_text(
        entropy_data->fe, entropy_data->success_template,
        "Key data has been created successfully."));
    printf("\n");
}

static void print_progress(struct entropy * entropy_data)
{
    int progress;

    progress = (double) (entropy_data->bytes_read) /
               (double) (entropy_data->keysize) * 100.0;
    if (entropy_data->last_progress < progress) {
        entropy_data->last_progress = progress;
        printf("\n---> %d%%\n", progress);
        if (100 == progress) {
            print_success(entropy_data);
        } else {
            print_action(entropy_data);
        }
    }
}

static int handle_input(struct entropy * entropy_data)
{
    int c;

    c = fgetc(stdin);
    if (entropy_data->fe->methods.can_go_back(entropy_data->fe,
                                              entropy_data->question)) {
        if (CHAR_GOBACK == c) {
            entropy_data->is_backing_up = DC_OK;
        } else if (('\n' == c || '\r' == c) && entropy_data->is_backing_up) {
            return DC_GOBACK;
        } else {
            entropy_data->is_backing_up = DC_NOTOK;
        }
    }
    return DC_OK;
}

static void wait_enter(void) {
    int c;

    do {
        c = fgetc(stdin);
    } while ('\n' != c && '\r' != c);
}

static int gather_entropy(struct entropy * entropy_data)
{
    struct termios oldt;
    struct termios newt;
    fd_set fds;

    print_progress(entropy_data);

    tcgetattr(0, &oldt);
    memcpy(&newt, &oldt, sizeof (struct termios));
    cfmakeraw(&newt);
    while (entropy_data->bytes_read < entropy_data->keysize) {
        tcsetattr(0, TCSANOW, &newt);
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(entropy_data->random_fd, &fds);
        if (-1 == select(entropy_data->random_fd + 1/* highest fd */,
                         &fds, NULL /* no write fds */,
                         NULL /* no except fds */, NULL /* no timeout */)) {
            error("select failed: %s", strerror(errno));
            return DC_NOTOK;
        }
        if (FD_ISSET(STDIN_FILENO, &fds)) {
            if (DC_GOBACK == handle_input(entropy_data)) {
                tcsetattr(0, TCSANOW, &oldt);
                return DC_GOBACK;
            }
            fputc('*', stdout);
        }
        tcsetattr(0, TCSANOW, &oldt);
        if (FD_ISSET(entropy_data->random_fd, &fds)) {
            move_bytes(entropy_data);
            print_progress(entropy_data);
        }
    }
    wait_enter();
    return DC_OK;
}

int cdebconf_text_handler_entropy(struct frontend * fe,
                                  struct question * question)
{
    struct entropy * entropy_data;
    int ret;

    if (NULL == (entropy_data = init_entropy(fe, question))) {
        error("init_entropy falied.");
        return DC_NOTOK;
    }
    if (!set_keysize(entropy_data, question)) {
        error("set_keysize failed.");
        ret = DC_NOTOK;
        goto out;
    }
    print_help(entropy_data);
    ret = gather_entropy(entropy_data);

out:
    destroy_entropy(entropy_data);
    return ret;
}
