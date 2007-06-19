/*
 * Data structure for representing http mirror information.
 * Contains essentially the same information as Mirrors.masterlist,
 * but only as much information as is necessary.
 */
struct mirror_t {
	char *site;
	char *country;
	char *root;
};

/* This is the codename of the preferred distribution; the one that the
 * current version of d-i is targeted at installing. This can be removed
 * once /etc/default_release is fully deployed. */
#define PREFERRED_DISTRIBUTION "lenny"

/* The string defined below must match the string used in the templates
 * (http and ftp) for this option. */
#define MANUAL_ENTRY "enter information manually"

#define SUITE_LENGTH 32

/* Stack of suites */
static const char suites[][SUITE_LENGTH] = {
	/* higher preference */
	PREFERRED_DISTRIBUTION,
	"stable",
	"testing",
	"unstable"
	/* lower preference */
};

#define DEBCONF_BASE "mirror/"
