#ifndef _QUESTION_H_
#define _QUESTION_H_

#define DC_QFLAG_SEEN		(1 << 0)
#define DC_QFLAG_DONTPARSE	(1 << 1)

struct template;

struct questionvariable {
	char *variable;
	char *value;
	struct questionvariable *next;
};

struct questionowner {
	char *owner;
	struct questionowner *next;
};

struct question {
	char *tag;
	unsigned int ref;
	char *value;
	char *defaultval;
	unsigned int flags;

	struct template *template;
	struct questionvariable *variables;
	struct questionowner *owners;
	struct question *prev, *next;
};

struct question *question_new(const char *tag);
void question_delete(struct question *question);
struct question *question_dup(struct question *q);

void question_ref(struct question *);
void question_deref(struct question *);

void question_setvalue(struct question *q, const char *value);

void question_variable_add(struct question *q, const char *var, 	
	const char *value);
void question_variable_delete(struct question *q, const char *var, 	
	const char *value);
void question_owner_add(struct question *q, const char *owner);
void question_owner_delete(struct question *q, const char *owner);
const char *question_description(struct question *q);
const char *question_extended_description(struct question *q);
const char *question_choices_translated(struct question *q);
const char *question_choices(struct question *q);
const char *question_defaultval(struct question *q);

#endif
