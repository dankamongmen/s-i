#ifndef _FRONTEND_NCURSES_H_
#define _FRONTEND_NCURSES_H_

#include <ncurses.h>

struct uidata {
};

static int nchandler_boolean(struct uidata *ui, struct question *q);
static int nchandler_multiselect(struct uidata *ui, struct question *q);
static int nchandler_note(struct uidata *ui, struct question *q);
static int nchandler_password(struct uidata *ui, struct question *q);
static int nchandler_select(struct uidata *ui, struct question *q);
static int nchandler_string(struct uidata *ui, struct question *q);
static int nchandler_text(struct uidata *ui, struct question *q);

#endif
