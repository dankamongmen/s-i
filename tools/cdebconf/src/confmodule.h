#ifndef _CONFMODULE_H_
#define _CONFMODULE_H_

#include "common.h"

struct configuration;
struct database;
struct frontend;

struct confmodule {
	struct configuration *config;
	struct database *db;
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
	struct database *, struct frontend *);
void confmodule_delete(struct confmodule *mod);

#endif
