/**
 *
 * @file commands.h
 * @brief Implementation of each command in the spec
 *
 */
#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include "confmodule.h"

#define CMDSTATUS_SUCCESS           0
#define CMDSTATUS_BADQUESTION       10
#define CMDSTATUS_BADPARAM          10
#define CMDSTATUS_SYNTAXERROR       20
#define CMDSTATUS_INPUTINVISIBLE    30
#define CMDSTATUS_BADVERSION        30
#define CMDSTATUS_GOBACK            30
#define CMDSTATUS_INTERNALERROR     100

/**
 * @brief Handler function type for each command
 * @param struct confmodule *mod - confmodule object
 * @param int argc - number of arguments
 * @param char **argv - argument array
 * @param char *out - output buffer
 * @param size_t outsize - output buffer size
 * @return int - DC_NOTOK if error, DC_OK otherwise
 */
typedef int (*command_function_t)(struct confmodule *mod, char *arg, char *out, size_t outsize);


/**
 * @brief mapping of command name to handler function
 */
typedef struct {
    const char *command;
    command_function_t handler;
} commands_t;

/**
 * @brief Handler for the INPUT debconf command
 *
 * Adds a question to the list of questions to be asked if appropriate.
 */
int command_input(struct confmodule *, char *, char *, size_t);

/**
 * @brief Handler for the CLEAR debconf command
 *
 * Removes any questions currently in the queue
 */
int command_clear(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the VERSION debconf command
 *
 * Checks to see if the version required by a confmodule script
 * is compatible with the debconf version we recognize
 */
int command_version(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the CAPB debconf command
 * 
 * Exchanges capability information between the confmodule and
 * the frontend
 */
int command_capb(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the TITLE debconf command
 *
 * Sets the title in the frontend
 */
int command_title(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the BEGINBLOCK debconf command
 * @warning Not yet implemented
 */
int command_beginblock(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the ENDBLOCK debconf command
 * @warning Not yet implemented
 */
int command_endblock(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the GO debconf command
 *
 * Asks all pending questions and save the answers into the debconf
 * database.
 *
 * Frontend should return CMDSTATUS_GOBACK only if the confmodule
 * supports backing up
 */
int command_go(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the GET debconf command
 *
 * Retrieves the value of a given template
 */
int command_get(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the SET debconf command
 *
 * Sets the value of a given template
 */
int command_set(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the RESET debconf command
 *
 * Resets the value of a given template to the default
 */
int command_reset(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the SUBST debconf command
 *
 * Registers a substitution variable/value for a template
 */
int command_subst(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the REGISTER debconf command
 */
int command_register(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the UNREGISTER debconf command
 */
int command_unregister(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the PURGE debconf command
 *
 * Removes all questions owned by a given owner
 */
int command_purge(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the METAGET debconf command
 *
 * Retrieves a given attribute for a template
 */
int command_metaget(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the FGET debconf command
 * 
 * Retrieves a given flag value for a template
 */
int command_fget(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the FSET debconf command
 * 
 * Sets a given flag value for a template
 */
int command_fset(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the EXISTS debconf command
 *
 * Checks to see if a template exists
 */
int command_exist(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the STOP debconf command
 *
 * Finishes the debconf session
 */
int command_stop(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the PROGRESS debconf command
 *
 * Progress bar handling
 *
 * @warning This is not yet in the debconf spec
 */
int command_progress(struct confmodule *, char *, char *, size_t);

/**
 * @brief handler for the X_LOADTEMPLATE debconf command
 *
 * Loads a new template into the debconf database
 *
 * @warning This is not in the debconf spec
 */
int command_x_loadtemplatefile(struct confmodule *, char *, char *, size_t);

#endif
