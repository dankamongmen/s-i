/**
 * @file question.c
 * @brief interfaces for handling debconf questions
 */
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
	unsigned int flags;

	struct template *template;
	struct questionvariable *variables;
	struct questionowner *owners;
	struct question *prev, *next;

        char *priority;
};

struct question *question_new(const char *tag);
void question_delete(struct question *question);

/**
 * @brief duplicate a question
 * @param q - the question to be duplicated
 * @return a deep copy of the question struct passed as input.  the 
 *         template pointer is not changed
 */
struct question *question_dup(struct question *q);

void question_ref(struct question *);
void question_deref(struct question *);

void question_setvalue(struct question *q, const char *value);
char *question_getvalue(struct question *q, const char *lang);

void question_variable_add(struct question *q, const char *var, 	
	const char *value);
void question_variable_delete(struct question *q, const char *var, 	
	const char *value);
void question_owner_add(struct question *q, const char *owner);
void question_owner_delete(struct question *q, const char *owner);
char *question_get_field(struct question *q, const char *lang,
	const char *field);

#endif
