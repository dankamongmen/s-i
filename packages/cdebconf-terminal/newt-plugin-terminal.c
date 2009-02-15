/*
 * cdebconf newt plugin to provide a clean terminal
 *
 * Copyright (C) 2009 Canonical Ltd.
 * Written by Colin Watson.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>

#include <newt.h>

#include <debian-installer.h>

#include <cdebconf/frontend.h>
#include <cdebconf/question.h>
#include <cdebconf/cdebconf_newt.h>

#define MODULE "terminal"

#define error(fmt, args...) syslog(LOG_ERR, MODULE ": " fmt, ##args)

#define DEFAULT_COMMAND_LINE "/bin/sh"

void cleanup_bterm_terminfo(char *tempdir);
char *setup_bterm_terminfo(void);
int cdebconf_newt_handler_terminal(struct frontend *obj, struct question *q);

/* Best effort to clean up after setup_bterm_terminfo. Does not error; if it
 * fails then some junk temporary files will be left behind. Frees tempdir.
 */
void
cleanup_bterm_terminfo(char *tempdir)
{
    DIR *dir = opendir(tempdir);
    struct dirent *dirent;

    if (!dir) {
        free(tempdir);
        return;
    }
    while ((dirent = readdir(dir)) != NULL) {
        char *path;
        struct stat st;

        if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
            continue;

        if (asprintf(&path, "%s/%s", tempdir, dirent->d_name) < 0)
            continue;
        if (lstat(path, &st) < 0)
            continue;
        if (S_ISDIR(st.st_mode))
            cleanup_bterm_terminfo(path); /* also frees path */
        else {
            unlink(path);
            free(path);
        }
    }

    closedir(dir);
    rmdir(tempdir);
    free(tempdir);
}

/* Astonishingly inconvenient pain to arrange for TERM=bterm to work in
 * /target. This would be a lot easier if bogl-bterm's terminfo definition
 * were moved into ncurses. Returns the name of a temporary directory that
 * must be cleaned up.
 */
char *
setup_bterm_terminfo(void)
{
    struct stat st;
    char *tempdir = NULL;
    char *targetfile;
    FILE *source = NULL, *target = NULL;
    char buf[4096];
    size_t r;

#define FAIL(fmt, args...) do { error(fmt, ##args); goto fail; } while (0)

    /* Debian- and d-i-specific paths follow. Sorry. */

    if (stat("/target/bin/sh", &st) < 0)
        /* We don't seem to be working with a target system. */
        return NULL;

    source = fopen("/usr/share/terminfo/b/bterm", "r");
    if (!source)
        /* We don't know about bterm either, so can't do anything. */

    if (stat("/target/usr/share/terminfo/b/bterm", &st) == 0)
        /* The target system already knows about bterm. */
        goto fail;
    
    tempdir = strdup("/target/tmp/cdebconf-terminal.XXXXXX");
    if (!tempdir)
        FAIL("strdup failed: %s", strerror(errno));
    tempdir = mkdtemp(tempdir);
    if (!tempdir)
        FAIL("mkdtemp failed: %s", strerror(errno));
    if (asprintf(&targetfile, "%s/b", tempdir) < 0)
        FAIL("asprintf failed: %s", strerror(errno));
    if (mkdir(targetfile, 0700) < 0)
        FAIL("mkdir(%s) failed: %s", targetfile, strerror(errno));
    if (asprintf(&targetfile, "%s/b/bterm", tempdir) < 0)
        FAIL("asprintf failed: %s", strerror(errno));
    target = fopen(targetfile, "w");
    if (!target)
        FAIL("fopen(%s, \"w\") failed: %s", targetfile, strerror(errno));

    while ((r = fread(buf, 1, 4096, source)) > 0) {
        size_t w = fwrite(buf, 1, r, target);
        if (w < r)
            FAIL("short write to %s", targetfile);
    }
    if (ferror(source))
        FAIL("error reading from /usr/share/terminfo/b/bterm");
    fclose(target);
    fclose(source);

#undef FAIL

    return tempdir;

fail:
    if (target)
        fclose(target);
    if (tempdir)
        cleanup_bterm_terminfo(tempdir);
    if (source)
        fclose(source);
    return NULL;
}

/* But we already have a terminal!
 *
 * Well, not quite. Firstly, we have to disconnect from debconf (which isn't
 * too hard; the debconf-disconnect utility in di-utils does it, although in
 * a cdebconf plugin there's no need for this since our standard file
 * descriptors are already as desired). Secondly, the terminal type is
 * "bterm", which the installed system doesn't know about; this causes some
 * inconveniences. These problems can more or less be handled outside
 * cdebconf if all we need is to start a shell.
 *
 * Much more significantly, though, newt is probably already displaying a
 * window. In order to do a good enough job to be able to run a full-screen
 * curses application such as aptitude, we need to tell it to tear
 * everything down, and then we need to put it all back so that it doesn't
 * look dreadful when we exit (particularly if e.g. a progress bar is being
 * displayed).
 */
int
cdebconf_newt_handler_terminal(struct frontend *obj, struct question *q)
{
    char *saved_progress_title = NULL;
    int saved_progress_min, saved_progress_max, saved_progress_cur;
    char *saved_progress_info = NULL;
    const char *command_line;
    const char *term;
    char *bterm_tempdir = NULL;
    struct sigaction sa, osa_sigchld;
    pid_t pid;
    int status;

    /* Tear down newt. */
    newtPopHelpLine();
    if (obj->progress_title)
        saved_progress_title = strdup(obj->progress_title);
    saved_progress_min = obj->progress_min;
    saved_progress_max = obj->progress_max;
    saved_progress_cur = obj->progress_cur;
    saved_progress_info = cdebconf_newt_get_progress_info(obj);
    newtFinished();

    command_line = question_get_variable(q, "COMMAND_LINE");
    if (NULL == command_line)
        command_line = DEFAULT_COMMAND_LINE;

    term = getenv("TERM");
    if (term && !strcmp(term, "bterm"))
        /* ncurses in /target may not have a terminfo definition for bterm.
         * Try to give it one.
         */
        bterm_tempdir = setup_bterm_terminfo();

    /* cdebconf handles SIGCHLD so that it can be notified of confmodule
     * death. We don't want that here. (Maybe cdebconf should be more
     * specific about its waitpid instead?)
     */
    memset (&sa, 0, sizeof sa);
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction (SIGCHLD, &sa, &osa_sigchld) < 0) {
        error("can't set SIGCHLD disposition to default: %s", strerror(errno));
        return DC_NOTOK;
    }

    pid = fork();
    if (-1 == pid) {
        error("can't fork: %s", strerror(errno));
        return DC_NOTOK;
    }
    if (0 == pid) {
        /* Child process. Remove all debconf-ish environment variables,
         * redirect file descriptors, and exec.
         */
        size_t cur_env = 0, max_env = 16;
        char **newenviron = di_new(char *, max_env), **envp;
        char *argv[4];

#define ENSURE_ENV do { \
    if (cur_env >= max_env) { \
        max_env *= 2; \
        newenviron = di_renew(char *, newenviron, max_env); \
    } \
} while (0)

        for (envp = environ; envp && *envp; ++envp) {
            if (strncmp(*envp, "DEBIAN_", 7) != 0 &&
                strncmp(*envp, "DEBCONF_", 8) != 0) {
                ENSURE_ENV;
                newenviron[cur_env++] = strdup(*envp);
            }
        }
        if (bterm_tempdir) {
            ENSURE_ENV;
	    /* bterm_tempdir always starts with "/target"; remove that,
	     * since the point of the exercise is to make things work for
	     * programs chrooted into /target within d-i. Programs outside
	     * the chroot will already have a working terminfo definition
	     * anyway.
	     */
            if (asprintf(&newenviron[cur_env++],
                         "TERMINFO=%s", bterm_tempdir + 7) < 0) {
                /* Not a lot we can do; let's just go on. */
            }
	    /* UTF-8 line-drawing characters appear not to work in bterm.
	     * ncurses disables these for some known-broken cases such as
	     * TERM=linux; tell it to do so here as well.
	     */
	    ENSURE_ENV;
	    newenviron[cur_env++] = strdup("NCURSES_NO_UTF8_ACS=1");
        }
        ENSURE_ENV;
        newenviron[cur_env] = NULL;

#undef ENSURE_ENV

        argv[0] = strdup("sh");
        argv[1] = strdup("-c");
        argv[2] = strdup(command_line);
        argv[3] = NULL;
        execve("/bin/sh", argv, newenviron);
        exit(127);
    }

    /* Parent process. */
    if (waitpid(pid, &status, 0) < 0)
        error("waitpid failed: %s", strerror(errno));

    /* Restore SIGCHLD handling. */
    sigaction(SIGCHLD, &osa_sigchld, NULL);

    if (bterm_tempdir)
        cleanup_bterm_terminfo(bterm_tempdir);

    /* Set newt back up again. */
    cdebconf_newt_setup();
    if (saved_progress_title) {
        obj->methods.progress_start(obj,
                                    saved_progress_min, saved_progress_max,
                                    saved_progress_title);
        free(saved_progress_title);
        obj->methods.progress_set(obj, saved_progress_cur);
        if (saved_progress_info) {
            obj->methods.progress_info(obj, saved_progress_info);
            free(saved_progress_info);
        }
    }

    if (status != 0)
        return DC_NOTOK;
    else
        return DC_OK;
}
