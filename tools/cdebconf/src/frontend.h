#ifndef _FRONTEND_H_
#define _FRONTEND_H_

struct configuration;
struct template_db;
struct question_db;
struct question;
struct frontend;

#define DCF_CAPB_BACKUP		(1UL << 0)

struct frontend_module {
    int (*initialize)(struct frontend *, struct configuration *);
    int (*shutdown)(struct frontend *);
    unsigned long (*query_capability)(struct frontend *);
    void (*set_title)(struct frontend *, const char *title);
    int (*add)(struct frontend *, struct question *q);
    int (*go)(struct frontend *);
    void (*clear)(struct frontend *);
    int (*can_go_back)(struct frontend *, struct question *);
    int (*can_go_forward)(struct frontend *, struct question *);
};

struct frontend {
	/* module handle */
	void *handle;
	/* configuration data */
	struct configuration *config;
    /* config path - base of instance configuration */
    char configpath[DEBCONF_MAX_CONFIGPATH_LEN];
	/* database handle for templates and config */
	struct template_db *tdb;
	struct question_db *qdb;
	/* frontend capabilities */
	unsigned long capability;
	/* private data */
	void *data;

	/* class variables */
	struct question *questions;
	int interactive;
	char *capb;
	char *title;
	
	/* methods */
    struct frontend_module methods;
};

struct frontend *frontend_new(struct configuration *, struct template_db *, struct question_db *);
void frontend_delete(struct frontend *);

#endif
