#ifndef _CONFMODULE_H_
#define _CONFMODULE_H_

#include "common.h"

struct configuration;
struct template_db;
struct question_db;
struct frontend;

struct confmodule {
	struct configuration *config;
	struct template_db *templates;
    struct question_db *questions;
	struct frontend *frontend;
	pid_t pid;
	int infd, outfd;
	int exitcode;
	char *owner;

	/* methods */
	int (*run)(struct confmodule *, int argc, char **argv);
	int (*communicate)(struct confmodule *mod);
	int (*shutdown)(struct confmodule *mod);
};

struct confmodule *confmodule_new(struct configuration *,
	struct template_db *, struct question_db *, struct frontend *);
void confmodule_delete(struct confmodule *mod);

#endif
