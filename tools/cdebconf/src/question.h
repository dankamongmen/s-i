#ifndef _QUESTION_H_
#define _QUESTION_H_

#define DC_QFLAG_ISDEFAULT		(1 << 0)
#define DC_QFLAG_HASSEEN		DC_QFLAG_ISDEFAULT

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
	unsigned int ref;
	char *tag;
	char *value;
	char *defaultval;
	unsigned int flags;

	struct template *template;
	struct questionvariable *variables;
	struct questionowner *owners;
	struct question *next;
};

struct question *question_new(const char *tag);
void question_delete(struct question *question);

void question_ref(struct question *);
void question_deref(struct question *);

void question_setvalue(struct question *q, const char *value);

void question_variable_add(struct question *q, const char *var, 	
	const char *value);
void question_variable_delete(struct question *q, const char *var, 	
	const char *value);
void question_owner_add(struct question *q, const char *owner);
void question_owner_delete(struct question *q, const char *owner);

#endif
