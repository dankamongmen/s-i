/**
 * @file frontend.h
 * @brief debconf frontend interface
 */
#ifndef _FRONTEND_H_
#define _FRONTEND_H_

#include <stdbool.h>

#undef _
#define _(x) (x)

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
    bool (*can_go_back)(struct frontend *, struct question *);
    bool (*can_go_forward)(struct frontend *, struct question *);

    void (*progress_start)(struct frontend *fe, int min, int max, const char *title);
    void (*progress_set) (struct frontend *fe, int val);
    /* You do not have to implement _step, it will call _set by default */
    void (*progress_step)(struct frontend *fe, int step);
    void (*progress_info)(struct frontend *fe, const char *info);
    void (*progress_stop)(struct frontend *fe);
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
	char *progress_title;
    int progress_min, progress_max, progress_cur;
	
	/* methods */
    struct frontend_module methods;
};

struct frontend *frontend_new(struct configuration *, struct template_db *, struct question_db *);
void frontend_delete(struct frontend *);

#endif
