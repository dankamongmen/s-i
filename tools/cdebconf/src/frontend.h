#ifndef _FRONTEND_H_
#define _FRONTEND_H_

struct configuration;
struct database;
struct question;
struct frontend;

#define DCF_CAPB_BACKUP		(1UL << 0)

/* frontend method function prototypes */
typedef int (*dcf_initialize_t)(struct frontend *, struct configuration *);
typedef int (*dcf_shutdown_t)(struct frontend *);
typedef unsigned long (*dcf_query_capability_t)(struct frontend *);
typedef void (*dcf_set_title_t)(struct frontend *, const char *title);
typedef int (*dcf_add_t)(struct frontend *, struct question *q);
typedef int (*dcf_go_t)(struct frontend *);
typedef void (*dcf_clear_t)(struct frontend *);
typedef int (*dcf_cangoback_t)(struct frontend *, struct question *);
typedef int (*dcf_cangoforward_t)(struct frontend *, struct question *);

struct frontend {
	/* module handle */
	void *handle;
	/* configuration data */
	struct configuration *config;
	/* database handle */
	struct database *db;
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
	dcf_initialize_t initialize;
	dcf_shutdown_t shutdown;
	dcf_query_capability_t query_capability;
	dcf_set_title_t set_title;
	dcf_add_t add;
	dcf_go_t go;
	dcf_clear_t clear;
	dcf_cangoback_t cangoback;
	dcf_cangoforward_t cangoforward;
};

struct frontend *frontend_new(struct configuration *, struct database *);
void frontend_delete(struct frontend *);

struct frontend_module {
	dcf_initialize_t initialize;
	dcf_shutdown_t shutdown;
	dcf_query_capability_t query_capability;
	dcf_set_title_t set_title;
	dcf_add_t add;
	dcf_go_t go;
	dcf_clear_t clear;
	dcf_cangoback_t cangoback;
	dcf_cangoforward_t cangoforward;
};

#endif
