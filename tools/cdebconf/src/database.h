#ifndef _DATABASE_H_
#define _DATABASE_H_

/* Debconf database interface */

struct configuration;
struct question;
struct database;
struct template;

/* Method prototypes */
typedef int (*dcd_initialize_t)(struct database *, struct configuration *);
typedef int (*dcd_shutdown_t)(struct database *);
typedef int (*dcd_load_t)(struct database *);
typedef int (*dcd_save_t)(struct database *);
typedef int (*dcd_template_set_t)(struct database *, struct template *t);
typedef struct template *(*dcd_template_get_t)(struct database *, 
	const char *name);
typedef int (*dcd_template_remove_t)(struct database *, const char *name);
typedef int (*dcd_template_lock_t)(struct database *, const char *name);
typedef int (*dcd_template_unlock_t)(struct database *, const char *name);
typedef struct template *(*dcd_template_iterate_t)(struct database *,
		void **iter);
typedef struct question *(*dcd_question_get_t)(struct database *, 
	const char *name);
typedef int (*dcd_question_set_t)(struct database *, struct question *q);
typedef int (*dcd_question_disown_t)(struct database *, const char *name, 
	const char *owner);
typedef int (*dcd_question_disownall_t)(struct database *, const char *owner);
typedef int (*dcd_question_lock_t)(struct database *, const char *name);
typedef int (*dcd_question_unlock_t)(struct database *, const char *name);
typedef int (*dcd_question_visible_t)(struct database *, const char *name, 
	const char *priority);
typedef struct question *(*dcd_question_iterate_t)(struct database *,
		void **iter);

struct database {
	/* db module name */
	const char *modname;
	/* db module handle */
	void *handle;
	/* configuration data */
	struct configuration *config;
	/* private data */
	void *data; 

	/* methods */
	dcd_initialize_t initialize;
	dcd_shutdown_t shutdown;
	dcd_load_t load;
	dcd_save_t save;
	dcd_template_set_t template_set;
	dcd_template_get_t template_get;
	dcd_template_remove_t template_remove;
	dcd_template_lock_t template_lock;
	dcd_template_unlock_t template_unlock;
	dcd_template_iterate_t template_iterate;
	dcd_question_get_t question_get;
	dcd_question_set_t question_set;
	dcd_question_disown_t question_disown;
	dcd_question_disownall_t question_disownall;
	dcd_question_lock_t question_lock;
	dcd_question_unlock_t question_unlock;
	dcd_question_visible_t question_visible;
	dcd_question_iterate_t question_iterate;
};

struct database *database_new(struct configuration *);
void database_delete(struct database *db);

struct database_module {
	dcd_initialize_t initialize;
	dcd_shutdown_t shutdown;
	dcd_load_t load;
	dcd_save_t save;
	dcd_template_set_t template_set;
	dcd_template_get_t template_get;
	dcd_template_remove_t template_remove;
	dcd_template_lock_t template_lock;
	dcd_template_unlock_t template_unlock;
	dcd_template_iterate_t template_iterate;
	dcd_question_get_t question_get;
	dcd_question_set_t question_set;
	dcd_question_disown_t question_disown;
	dcd_question_disownall_t question_disownall;
	dcd_question_lock_t question_lock;
	dcd_question_unlock_t question_unlock;
	dcd_question_visible_t question_visible;
	dcd_question_iterate_t question_iterate;
};

#endif
