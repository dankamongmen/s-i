#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

struct template
{
	unsigned int ref;
	char *tag;
	char *type;
	char *defaultval;
	char *choices;
	char *description;
	char *extended_description;
	struct template *next;
};

struct template *template_new(const char *tag);
void template_delete(struct template *t);
void template_ref(struct template *t);
void template_deref(struct template *t);

struct template *template_load(const char *filename);

#endif
