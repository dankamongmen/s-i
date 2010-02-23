/**
 * @file dpkg-reconfigure.c
 * @brief dpkg-reconfigure utility that allows users to 
 *        reconfigure a package after it's been installed
 */
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
#include <locale.h>
#include <stdbool.h>


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
	{ "default-priority", 0, NULL, 'd' },
	{ "all", 0, NULL, 'a' },
	{ "unseen-only", 0, NULL, 'u' },
	{ "force", 0, NULL, 'F' },
	{ 0, 0, 0, 0 }
};

static struct configuration *g_config;
static struct frontend *g_frontend;
static struct template_db *g_templates;
static struct question_db *g_questions;
static int opt_all, opt_force;

/************************************************************************
 * Function: cleanup
 * Inputs: none
 * Outputs: none
 * Description: cleans up if before we exit
 * Assumptions: none
 ************************************************************************/
static void cleanup(void)
{
	if (g_frontend != NULL)
		frontend_delete(g_frontend);
	if (g_templates != NULL)
	{
		g_templates->methods.save(g_templates);
		template_db_delete(g_templates);
	}
	if (g_questions != NULL)
	{
		g_questions->methods.save(g_questions);
		question_db_delete(g_questions);
	}
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
	printf("dpkg-reconfigure [--frontend <frontend>] [--priority <priority>] package\n");
	printf("dpkg-reconfigure [-f <frontend>] [-p <priority>] package\n");
	exit(0);
}

/************************************************************************
 * Function: file_exists
 * Inputs: filename - name of file to check
 *         mode - minimum mode requirement (mask)
 * Outputs: true if filename exists and is a regular file, false otherwise
 * Description: checks to see if a file exists
 * Assumptions: none
 ************************************************************************/
static bool file_exists(const char *filename, mode_t mode)
{
	struct stat buf;
	stat(filename, &buf);
	if (!S_ISREG(buf.st_mode)) return false;
	if ((buf.st_mode & mode) == 0) return false;
	return true;
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
		if (g_templates->methods.set(g_templates, t) != DC_OK)
			INFO(INFO_ERROR, "Cannot add template %s", t->tag);

		q = g_questions->methods.get(g_questions, t->tag);
		if (q == NULL)
		{
			q = question_new(t->tag);
			q->template = t;
			template_ref(t);
		}
		else if (q->template != t)
		{
			template_deref(q->template);
			q->template = t;
			template_ref(t);
		}
		question_owner_add(q, owner);
		if (g_questions->methods.set(g_questions, q) != DC_OK)
			INFO(INFO_ERROR, "Cannot add question %s", t->tag);
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

	confmodule = confmodule_new(g_config, g_templates, g_questions, g_frontend);
	confmodule->run(confmodule, argc, argv);
	confmodule->communicate(confmodule);
	confmodule->shutdown(confmodule);
	ret = confmodule->exitcode;
	confmodule_delete(confmodule);

	return ret;
}

int runscript (const char *pkg, const char *script, const char *param) {
	int ret;
	char filename[1024];
	char *argv[5] = {0};
	char *version = getfield(pkg, VERSIONFIELD);

	snprintf(filename, sizeof(filename), INFODIR "/%s.%s", pkg, script);
	if (! file_exists(filename,S_IXUSR|S_IXGRP|S_IXOTH))
		return 0; /* it's ok if the script doesn't exist */
	
	if (strcmp("script", "config") == 0 || is_confmodule(filename))
	{
		argv[1] = filename;
		/* not actually modified, but this ultimately ends up in
		 * execv(), which wants argv elements to be char *, so
		 * silence the warning
		 */
		argv[2] = (char *) param;
		argv[3] = version;
		if ((ret = runconfmodule(4, argv)) != 0)
			return ret;
	}
	else
	{
		/* according to debconf:
		 * Since a non confmodule might run other programs that
		 * use debconf, checkpoint the db state and
		 * reinitialize when the script finishes 
		 */
		g_templates->methods.save(g_templates);
		g_questions->methods.save(g_questions);
	
		unsetenv("DEBIAN_HAS_FRONTEND");

		strvacat(filename, sizeof(filename), " ", param, " ", version, NULL);
		ret = system(filename);
	
		setenv("DEBIAN_HAS_FRONTEND", "1", 1);
	
		g_templates->methods.load(g_templates);
		g_questions->methods.load(g_questions);
	}

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
	char filename[1024];
	char *pkg;

	while (i < max)
	{
		pkg = pkgs[i++];

		g_frontend->methods.set_title(g_frontend, pkg);
		if (strstr(getfield(pkg, STATUSFIELD), " ok installed") == 0)
			DIE("%s is not fully installed", pkg);

		snprintf(filename, sizeof(filename), INFODIR "/%s.templates", pkg);
		if (file_exists(filename, S_IRUSR|S_IRGRP|S_IROTH))
			loadtemplate(filename, pkg);
		
		/* Simulation of reinstalling a package, without bothering with
		   removing the files and putting them back. Just like in a
		   regular reinstall, run config, and postinst scripts in
		   sequence, with args. Do not run postrm, because the postrm
		   can depend on the package's files actually being gone
		   already. */
		if ((ret = runscript(pkg, "prerm", "upgrade")) != 0)
			return ret;
		if ((ret = runscript(pkg, "config", "reconfigure")) != 0)
			return ret;
		if ((ret = runscript(pkg, "postinst", "configure")) != 0)
			return ret;

		i++;
	}
	return 0;
}

/************************************************************************
 * Function: main
 * Inputs: argc, argv - arguments passed in
 * Outputs: int - 0 if no error, >0 otherwise
 * Description: main entry point to dpkg-reconfigure
 * Assumptions: none
 ************************************************************************/
int main(int argc, char **argv)
{
	int opt, ret;
	int unseen_only=0;
	int default_priority=0;
	char *priority_override=NULL;

	signal(SIGINT, sighandler);
	setlocale (LC_ALL, "");

	if (getuid() != 0)
		DIE("%s must be run as root", argv[0]);

	g_config = config_new();

	while ((opt = getopt_long(argc, argv, "hf:p:duaF", g_dpc_args, NULL)) > 0)
	{
		switch (opt)
		{
		case 'h': usage(); break;
		case 'f': setenv("DEBIAN_FRONTEND", optarg, 1); break;
		case 'p': priority_override=strdup(optarg); break;
		case 'd': default_priority=1; break;
		case 'u': unseen_only=1; break;
		case 'a': opt_all = 1; break;
		case 'F': opt_force = 1; break;
		}
	}
	if (optind == argc) {
		fprintf(stderr, "please specify a package to reconfigure\n");
		exit(1);
	}
	
	/* Default is to force showing of old questions by default
	 * for reconfiguring, and show low priority questions. */
	if (! unseen_only)
		g_config->set(g_config, "_cmdline::showold", "true");
	if (priority_override)
		g_config->set(g_config, "_cmdline::priority", priority_override);
	else if (! default_priority && (getenv("DEBIAN_PRIORITY") == NULL))
		g_config->set(g_config, "_cmdline::priority", "low");

	/* parse the configuration info */
	if (g_config->read(g_config, DEBCONFCONFIG) == 0)
		DIE("Error reading configuration information");

	/* initialize database and frontend modules */
	if ((g_templates = template_db_new(g_config, NULL)) == 0)
		DIE("Cannot initialize DebConf templates database");
	if ((g_questions = question_db_new(g_config, g_templates, NULL)) == 0)
		DIE("Cannot initialize DebConf config database");
	if ((g_frontend = frontend_new(g_config, g_templates, g_questions)) == 0)
		DIE("Cannot initialize DebConf frontend");

	g_templates->methods.load(g_templates);
	g_questions->methods.load(g_questions);

	setenv("DEBIAN_HAS_FRONTEND", "1", 1);
	ret = reconfigure(argv, optind, argc);
	/* shutting down .... sync the database and shutdown the modules */
	cleanup();
	return ret;
}
