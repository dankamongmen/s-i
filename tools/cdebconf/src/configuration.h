#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

/* This is roughly based on the APT configuration class */

struct configitem {
	char *tag;
	char *value;
	struct configitem *parent, *child, *next;
};

struct configuration {
	struct configitem *root;

	const char *(*get)(struct configuration *, const char *tag, 
		const char *defaultvalue);
	int (*geti)(struct configuration *, const char *tag, 
		int defaultvalue);
	void (*set)(struct configuration *, const char *tag,
		const char *value);
	void (*seti)(struct configuration *, const char *tag,
		int value);
	int (*exists)(struct configuration *, const char *tag);

	int (*read)(struct configuration *, const char *filename);
	void (*dump)(struct configuration *);

	struct configitem *(*tree)(struct configuration *, const char *tag);
};

struct configuration *config_new(void);
void config_delete(struct configuration *);

#endif
