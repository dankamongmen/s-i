/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: confmodule.c
 *
 * Description: main configuration module interface; handles running
 *              of client configuration modules and communications
 *              between the debconf frontend and the confmodule
 *
 * $Id: confmodule.c,v 1.13 2002/07/01 06:58:37 tausq Exp $
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
#include "confmodule.h"
#include "commands.h"
#include "frontend.h"
#include "strutl.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

static commands_t commands[] = {
	{ "input",	command_input },
	{ "clear",	command_clear },
	{ "version",	command_version },
	{ "capb",	command_capb },
	{ "title",	command_title },
	{ "beginblock",	command_beginblock },
	{ "endblock", 	command_endblock },
	{ "go",		command_go },
	{ "get",	command_get },
	{ "set",	command_set },
	{ "reset",	command_reset },
	{ "subst",	command_subst },
	{ "register",	command_register },
	{ "unregister",	command_unregister },
	{ "purge",	command_purge },
	{ "metaget",	command_metaget },
	{ "fget",	command_fget },
	{ "fset",	command_fset },
	{ "exist",	command_exist },
	{ "stop",	command_stop },
        { "x_loadtemplatefile", command_x_loadtemplatefile },
	{ 0, 0 }
};

/* private functions */
/*
 * Function: _confmodule_process
 * Input: struct confmodule *mod - confmodule object
 *        char *in - input command
 *        char *out - reply buffer
 *        size_t outsize - reply buffer length
 * Output: int - DC_OK, DC_NOTOK, DC_NOTIMPL
 * Description: helper function to process incoming commands
 * Assumptions: in != NULL
 */
static int _confmodule_process(struct confmodule *mod, char *in, char *out, size_t outsize)
{
	int i = 0, argc;
	char *argv[10];

	out[0] = 0;
	if (*in == '#') return 1;

	memset(argv, 0, sizeof(char *) * DIM(argv));
	argc = strcmdsplit(in, argv, DIM(argv) - 1);

	for (; commands[i].command != 0; i++)
	{
		if (strcasecmp(argv[0], commands[i].command) == 0) {
			return (*commands[i].handler)(mod, argc - 1, argv, 
							out, outsize);
		}
	}
	return DC_NOTOK;
}

/* public functions */
/*
 * Function: confmodule_communicate
 * Input: struct confmodule *mod - confmodule object
 * Output: int - DC_OK, DC_NOTOK
 * Description: handles communication between a config script and the
 *              confmodule
 * Assumptions: none
 */
static int confmodule_communicate(struct confmodule *mod)
{
	char in[1024];
	char out[1024];
	char *inp;
	int ret = 0;

	while ((ret = read(mod->infd, in, sizeof(in))) > 0)
	{
		in[ret] = 0;
		inp = strstrip(in);
		INFO(INFO_DEBUG, "--> %s\n", inp);
		ret = _confmodule_process(mod, inp, out, sizeof(out) - 1);
		if (ret == DC_NOTIMPL) {
			fprintf(stderr, "E: Unimplemented function\n");
			continue;
		}
		if (out[0] == 0) break; // STOP called
		INFO(INFO_DEBUG, "<-- %s\n", out);
		strcat(out, "\n");
		write(mod->outfd, out, strlen(out));
	}
	return ret;
}

/*
 * Function: confmodule_shutdown
 * Input: struct confmodule *mod - confmodule object
 * Output: int - exit code of the config script
 * Description: Shuts down the confmodule
 * Assumptions: none
 */
static int confmodule_shutdown(struct confmodule *mod)
{
	int status;

	mod->exitcode = 0;

	waitpid(mod->pid, &status, 0);

	if (WIFEXITED(status))
		mod->exitcode = WEXITSTATUS(status);
	
	return mod->exitcode;
}

/*
 * Function: confmodule_run
 * Input: struct confmodule *mod - confmodule object
 *        int argc - number of arguments to pass to config script
 *        char **argv - argument array
 * Output: int - pid of config script, -1 if error
 * Description: runs a config script, connected to the confmodule
 * Assumptions: none
 */
static int confmodule_run(struct confmodule *mod, int argc, char **argv)
{
	int pid;
	int i;
	char **args;
	int toconfig[2], fromconfig[2]; /* 0=read, 1=write */
	pipe(toconfig);
	pipe(fromconfig);
	switch ((pid = fork()))
	{
	case -1:
		mod->frontend->methods.shutdown(mod->frontend);
		DIE("Cannot execute client config script");
		break;
	case 0:
		close(fromconfig[0]); close(toconfig[1]);
		if (toconfig[0] != 0) { /* if stdin is closed initially */
			dup2(toconfig[0], 0); close(toconfig[0]);
		}
		dup2(fromconfig[1], 1); close(fromconfig[1]);

		args = (char **)malloc(sizeof(char *) * argc-1);
		for (i = 1; i < argc; i++)
			args[i-1] = argv[i];
		args[argc-1] = NULL;
		if (execv(argv[1], args) != 0)
			perror("execv");
		/* should never reach here, otherwise execv failed :( */
		DIE("Cannot execute client config script");
	default:
		close(fromconfig[1]); close(toconfig[0]);
		mod->infd = fromconfig[0];
		mod->outfd = toconfig[1];
	}

	return pid;
}

/*
 * Function: confmodule_new
 * Input: struct configuration *config - configuration parameters
 *        struct database *db - database object
 *        struct frontend *frontend - frontend UI object
 * Output: struct confmodule * - newly created confmodule
 * Description: creates a new confmodule object
 * Assumptions: none
 */
struct confmodule *confmodule_new(struct configuration *config,
	struct template_db *templates, struct question_db *questions, 
    struct frontend *frontend)
{
	struct confmodule *mod = NEW(struct confmodule);

	mod->config = config;
	mod->templates = templates;
	mod->questions = questions;
	mod->frontend = frontend;
	mod->run = confmodule_run;
	mod->communicate = confmodule_communicate;
	mod->shutdown = confmodule_shutdown;

	/* TODO: I wish we don't need gross hacks like this.... */
	setenv("DEBIAN_HAS_FRONTEND", "1", 1);

	return mod;
}

/*
 * Function: confmodule_delete
 * Input: confmodule *mod - confmodule object to destroy
 * Output: none
 * Description: destroys a confmodule object
 * Assumptions: none
 */
void confmodule_delete(struct confmodule *mod)
{
	DELETE(mod);
}
