#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include "confmodule.h"

#define CMDSTATUS_SUCCESS		0
#define CMDSTATUS_BADQUESTION		10
#define CMDSTATUS_BADPARAM		10
#define CMDSTATUS_SYNTAXERROR		20
#define CMDSTATUS_INPUTINVISIBLE	30
#define CMDSTATUS_BADVERSION		30
#define CMDSTATUS_GOBACK		30
#define CMDSTATUS_INTERNALERROR		100

typedef int (*command_function_t)(struct confmodule *, int, char **, char *, size_t);

typedef struct {
	const char *command;
	command_function_t handler;
} commands_t;

int command_input(struct confmodule *, int, char **, char *, size_t);
int command_clear(struct confmodule *, int, char **, char *, size_t);
int command_version(struct confmodule *, int, char **, char *, size_t);
int command_capb(struct confmodule *, int, char **, char *, size_t);
int command_title(struct confmodule *, int, char **, char *, size_t);
int command_beginblock(struct confmodule *, int, char **, char *, size_t);
int command_endblock(struct confmodule *, int, char **, char *, size_t);
int command_go(struct confmodule *, int, char **, char *, size_t);
int command_get(struct confmodule *, int, char **, char *, size_t);
int command_set(struct confmodule *, int, char **, char *, size_t);
int command_reset(struct confmodule *, int, char **, char *, size_t);
int command_subst(struct confmodule *, int, char **, char *, size_t);
int command_register(struct confmodule *, int, char **, char *, size_t);
int command_unregister(struct confmodule *, int, char **, char *, size_t);
int command_purge(struct confmodule *, int, char **, char *, size_t);
int command_metaget(struct confmodule *, int, char **, char *, size_t);
int command_fget(struct confmodule *, int, char **, char *, size_t);
int command_fset(struct confmodule *, int, char **, char *, size_t);
int command_exist(struct confmodule *, int, char **, char *, size_t);
int command_stop(struct confmodule *, int, char **, char *, size_t);
int command_x_loadtemplatefile(struct confmodule *, int, char **, char *, size_t);

#endif
