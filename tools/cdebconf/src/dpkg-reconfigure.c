/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: dpkg-reconfigure.c
 *
 * Description: dpkg-reconfigure utility that allows users to 
 *              reconfigure a package after it's been installed
 *
 * $Id: dpkg-reconfigure.c,v 1.8 2001/01/07 05:05:12 tausq Rel $
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "confmodule.h"
#include "configuration.h"
#include "frontend.h"
#include "database.h"
#include "question.h"
#include "template.h"
#include "strutl.h"

#define INFODIR		"/var/lib/dpkg/info"
#define STATUSFILE	"/var/lib/dpkg/status"
#define STATUSFIELD	"Status: "
#define VERSIONFIELD	"Version: "

struct option g_dpc_args[] = {
	{ "help", 0, NULL, 'h' },
	{ "frontend", 1, NULL, 'f' },
	{ "priority", 1, NULL, 'p' },
	{ "default-priority", 1, NULL, 'd' },
	{ "all", 0, NULL, 'a' },
	{ "unseen-only", 0, NULL, 'u' },
	{ 0, 0, 0, 0 }
};

static struct configuration *g_config = NULL;
static struct frontend *g_frontend = NULL;
static struct database *g_db = NULL;

/************************************************************************
 * Function: cleanup
 * Inputs: none
 * Outputs: none
 * Description: cleans up if before we exit
 * Assumptions: none
 ************************************************************************/
static void cleanup(void)
{
	if (g_db != NULL)
		g_db->save(g_db);
	if (g_frontend != NULL)
		frontend_delete(g_frontend);
	if (g_db != NULL)
		database_delete(g_db);
	if (g_config != NULL)
		config_delete(g_config);
}

/************************************************************************
 * Function: sighandler
 * Inputs: sig - signal caught
 * Outputs: none
 * Description: signal handler
 * Assumptions: only catches SIGINT right now
 ************************************************************************/
static void sighandler(int sig)
{
	cleanup();
}

/************************************************************************
 * Function: usage
 * Inputs: none
 * Outputs: none
 * Description: prints a usage message and exits
 * Assumptions: none
 ************************************************************************/
static void usage(void)
{
	printf("dpkg-reconfigure [--frontend <frontend>] [--priority <priority>]\n");
	printf("dpkg-reconfigure [-f <frontend>] [-p <priority>]\n");
	exit(0);
}

/************************************************************************
 * Function: file_exists
 * Inputs: filename - name of file to check
 *         mode - minimum mode requirement (mask)
 * Outputs: 1 if filename exists and is a regular file, 0 otherwise
 * Description: checks to see if a file exists
 * Assumptions: none
 ************************************************************************/
static int file_exists(const char *filename, mode_t mode)
{
	struct stat buf;
	stat(filename, &buf);
	if (!S_ISREG(buf.st_mode)) return 0;
	if ((buf.st_mode & mode) == 0) return 0;
	return 1;
}

/************************************************************************
 * Function: loadtemplate
 * Inputs: filename - which file to load templates from
 *         owner - owner for the templates
 * Outputs: none
 * Description: loads all the templates from a file
 * Assumptions: none
 ************************************************************************/
void loadtemplate(const char *filename, const char *owner)
{
	struct template *t;
	struct question *q;

	t = template_load(filename);
	while (t)
	{
		if (g_db->template_set(g_db, t) != DC_OK)
			INFO(INFO_ERROR, "Cannot add template %s", t->tag);

		q = g_db->question_get(g_db, t->tag);
		if (q == NULL)
		{
			q = question_new(t->tag);
			q->template = t;
		}
		question_owner_add(q, owner);
		if (g_db->question_set(g_db, q) != DC_OK)
			INFO(INFO_ERROR, "Cannot add template %s", t->tag);
		question_deref(q);
		t = t->next;
	}
}

/************************************************************************
 * Function: getfield
 * Inputs: package - which package to get the status of
 *         field - field to get value of
 * Outputs: char * - status of package
 * Description: gets the value of a field for a package from the status file
 * Assumptions: returns pointer to a static buffer
 ************************************************************************/
static char *getfield(const char *package, const char *fieldname)
{
	static char field[100] = {0};
	FILE *fp;
	char buf[1024], srch[256];
	int inpkg = 0;
	int fieldlen;

	field[0] = 0;
	snprintf(srch, sizeof(srch), "Package: %s", package);

	fieldlen = strlen(fieldname);

	if ((fp = fopen(STATUSFILE, "r")) != 0)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			CHOMP(buf);
			if (strcmp(buf, srch) == 0)
			{
				inpkg = 1;
			}
			else if (inpkg)
			{
				if (buf[0] == '\n')
				{
					inpkg = 0;
					continue;
				}
				if (strncmp(buf, fieldname, fieldlen) == 0)
				{
					strncpy(field, buf+fieldlen, sizeof(field));
					break;
				}
			}
		}
		fclose(fp);
	}
	return field;
}

/************************************************************************
 * Function: is_confmodule
 * Inputs: filename - filename to check
 * Outputs: 1 if filename is a confmodule, 0 otherwise
 * Description: "checks" to see if filename is a confmodule
 * Assumptions: filename is a confmodule if it contains the string
 *              "confmodule". This is ugly, gross, broken, etc but we
 *              don't have much choice, and we want this to be compatible
 *              with perl-debconf
 ************************************************************************/
static int is_confmodule(const char *filename)
{
	FILE *fp;
	char buf[1024];
	int i;
	int found = 0;

	if ((fp = fopen(filename, "r")) != 0)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			for (i = 0; buf[i] != 0; i++)
				buf[i] = tolower(buf[i]);
			if (strstr(buf, "confmodule"))
			{
				found = 1;
				break;
			}
		}
		fclose(fp);
	}
	return found;
}

/************************************************************************
 * Function: runconfmodule
 * Inputs: argc, argv - arguments for the confmodule
 * Outputs: int - status of running the confmodule
 * Description: runs the given confmodule
 * Assumptions: none
 ************************************************************************/
int runconfmodule(int argc, char **argv)
{
	struct confmodule *confmodule = NULL;
	int ret;

	confmodule = confmodule_new(g_config, g_db, g_frontend);
	confmodule->run(confmodule, argc, argv);
	confmodule->communicate(confmodule);
	ret = confmodule->exitcode;
	confmodule_delete(confmodule);

	return ret;
}

/************************************************************************
 * Function: reconfigure
 * Inputs: pkgs - pointer to an array of packages
 *         i - first index, max - max index
 * Outputs: int - 0 if no error, >0 otherwise
 * Description: Configure each of the packages passed in
 * Assumptions: none
 ************************************************************************/
int reconfigure(char **pkgs, int i, int max)
{
	int ret;
	char *argv[5] = {0};
	char filename[1024];
	char *pkg;

	while (i < max)
	{
		pkg = pkgs[i++];

		snprintf(filename, sizeof(filename), INFODIR "/%s.config", pkg);
		if (!file_exists(filename, S_IXUSR|S_IXGRP|S_IXOTH))
		{
			INFO(INFO_WARN, "%s is not installed, or does not use debconf", pkg);
			continue;
		}
		/* startup the confmodule; run the config script and talk to it */
		g_frontend->set_title(g_frontend, pkg);
		if (strstr(getfield(pkg, STATUSFIELD), " ok installed") == 0)
			DIE("%s is not fully installed", pkg);

		snprintf(filename, sizeof(filename), INFODIR "/%s.templates", pkg);
		if (file_exists(filename, S_IRUSR|S_IRGRP|S_IROTH))
			loadtemplate(filename, pkg);
		
		snprintf(filename, sizeof(filename), INFODIR "/%s.postinst", pkg);
		if (file_exists(filename,S_IXUSR|S_IXGRP|S_IXOTH))
		{
			if (is_confmodule(filename))
			{
				argv[1] = filename;
				argv[2] = "configure";
				argv[3] = getfield(pkg, VERSIONFIELD);
				if ((ret = runconfmodule(4, argv)) != 0)
					return ret;
			}
			else
			{
				g_db->save(g_db);
				strvacat(filename, sizeof(filename), " configure ", getfield(pkg, VERSIONFIELD), NULL);

				ret = system(filename);
				if (ret != 0) return DC_NOTOK;
				g_db->load(g_db);
			}
		}

		snprintf(filename, sizeof(filename), INFODIR "/%s.config", pkg);
		if (file_exists(filename,S_IXUSR|S_IXGRP|S_IXOTH))
		{
			argv[1] = filename;
			argv[2] = "reconfigure";
			argv[3] = getfield(pkg, VERSIONFIELD);
			if ((ret = runconfmodule(4, argv)) != DC_OK)
				return ret;
		}

		i++;
	}
	return 0;
}

/************************************************************************
 * Function: main
 * Inputs: argc, argv - arguments passed in
 * Outputs: int - 0 if no error, >0 otherwise
 * Description: main entry point to dpkg-preconfigure
 * Assumptions: none
 ************************************************************************/
int main(int argc, char **argv)
{
	int opt, ret;
	char *opt_frontend = NULL, *opt_priority = NULL;

	signal(SIGINT, sighandler);

	while ((opt = getopt_long(argc, argv, "dhf:p:", g_dpc_args, NULL) > 0))
	{
		switch (opt)
		{
		case 'h': usage(); break;
		case 'f': opt_frontend = optarg; break;
		case 'p': opt_priority = optarg; break;
		case 'd': break;
		case 'a': break;
		case 'u': break;
		}
	}

	g_config = config_new();

	/* parse the configuration info */
	if (g_config->read(g_config, DEBCONFCONFIG) == 0)
		DIE("Error reading configuration information");

	/* initialize database and frontend modules */
	if ((g_db = database_new(g_config)) == 0)
		DIE("Cannot initialize DebConf database");
	if ((g_frontend = frontend_new(g_config, g_db)) == 0)
		DIE("Cannot initialize DebConf frontend");

	g_db->load(g_db);
	setenv("DEBIAN_HAS_FRONTEND", "1", 1);
	ret = reconfigure(argv, optind, argc);
	/* shutting down .... sync the database and shutdown the modules */
	cleanup();
	return ret;
}
