#ifndef _DATABASE_H_
#define _DATABASE_H_

/**
 * \file database.h
 * \brief Definitions of question_db and template_db objects
 */


/* Debconf database interfaces */

struct configuration;
struct template;
struct template_db;
struct question;
struct question_db;

/**
 * Methods for a template database module
 */
struct template_db_module {
    int (*initialize)(struct template_db *db, struct configuration *cfg);
    int (*shutdown)(struct template_db *db);
    int (*load)(struct template_db *db);
    int (*save)(struct template_db *db);
    int (*set)(struct template_db *db, struct template *t);
    struct template *(*get)(struct template_db *db, const char *name);
    int (*remove)(struct template_db *, const char *name);
    int (*lock)(struct template_db *, const char *name);
    int (*unlock)(struct template_db *, const char *name);
    struct template *(*iterate)(struct template_db *db, void **iter);
};

/**
 * Methods for a question database module
 */
struct question_db_module {
    int (*initialize)(struct question_db *db, struct configuration *cfg);
    int (*shutdown)(struct question_db *db);
    int (*load)(struct question_db *db);
    int (*save)(struct question_db *db);
    int (*set)(struct question_db *, struct question *q);
    struct question *(*get)(struct question_db *db, const char *name);
    int (*disown)(struct question_db *, const char *name, const char *owner);
    int (*disownall)(struct question_db *, const char *owner);
    int (*lock)(struct question_db *, const char *name);
    int (*unlock)(struct question_db *, const char *name);
    int (*is_visible)(struct question_db *, const char *name, const char *priority);
    struct question *(*iterate)(struct question_db *, void **iter);
};

struct template_db {
	/* db module name */
	const char *modname;
	/* db module handle */
	void *handle;
	/* configuration data */
	struct configuration *config;
    /* config path - base of instance configuration */
    char configpath[DEBCONF_MAX_CONFIGPATH_LEN];
	/* private data */
	void *data; 

	/* methods */
    struct template_db_module methods;
};

struct question_db {
    /* db module name */
	const char *modname;
	/* db module handle */
	void *handle;
	/* configuration data */
	struct configuration *config;
    /* config path - base of instance configuration */
    char configpath[DEBCONF_MAX_CONFIGPATH_LEN];
	/* private data */
	void *data; 
    /* template database */
    struct template_db *tdb;

	/* methods */
    struct question_db_module methods;
};

struct template_db *template_db_new(struct configuration *);
void template_db_delete(struct template_db *db);

struct question_db *question_db_new(struct configuration *, struct template_db *tdb);
void question_db_delete(struct question_db *db);

#endif
