#define _GNU_SOURCE /* for getopt_long */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <syslog.h>
#include <debian-installer.h>

#ifdef __GNUC__
#  define ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
#  define ATTRIBUTE_UNUSED
#endif

int pass(const char *buf, size_t len ATTRIBUTE_UNUSED, void *user_data ATTRIBUTE_UNUSED)
{
	/* di_exec_* always strips any trailing newline, so we have to add
	 * one back on; we hope that nothing depends on the subsidiary
	 * program *not* outputting a trailing newline!
	 */
	printf("%s\n", buf);
	return 0;
}

int logger(const char *buf, size_t len ATTRIBUTE_UNUSED, void *user_data)
{
	static int log_open = 0;

	if (!log_open) {
		const char *ident = (const char *) user_data;
		if (!ident)
			ident = "log-output";
		openlog(ident, 0, LOG_USER);
		log_open = 1;
	}

	syslog(LOG_NOTICE, "%s", buf);

	return 0;
}

void usage(FILE *output)
{
	fprintf(output, "Usage: log-output -t TAG [--pass-stdout] PROGRAM [ARGUMENTS]\n");
}

int main(int argc, char **argv)
{
	char *tag = NULL;
	static int pass_stdout = 0;
	static struct option long_options[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "pass-stdout", no_argument, &pass_stdout, 1 },
		{ NULL, 0, NULL, 0 }
	};
	di_io_handler *stdout_handler = NULL, *stderr_handler = NULL;
	int status;

	for (;;) {
		int c = getopt_long(argc, argv, "+t:", long_options, NULL);
		if (c == -1)
			break;
		switch (c) {
			case 0: /* long option */
				break;
			case 'h':
				usage(stdout);
				break;
			case 't':
				tag = strdup(optarg);
				break;
			default:
				usage(stderr);
				exit(1);
		}
	}

	if (!argv[optind])
		return 0;

	if (pass_stdout)
		stdout_handler = &pass;
	else
		stdout_handler = &logger;
	stderr_handler = &logger;
	status = di_exec_path_full(argv[optind], (const char **) &argv[optind],
		stdout_handler, stderr_handler, tag, NULL, NULL, NULL, NULL);

	return di_exec_mangle_status(status);
}
