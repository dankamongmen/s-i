#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

struct language_description 
{
	char *language;
	char *description;
	char *extended_description;
	char *choices;
	struct language_description *next;
};

struct template
{
	char *tag;
	unsigned int ref;
	char *type;
	char *defaultval;
	char *choices;
	char *description;
	char *extended_description;
	struct language_description *localized_descriptions;
	struct template *next;
};

struct template *template_new(const char *tag);
void template_delete(struct template *t);
void template_ref(struct template *t);
void template_deref(struct template *t);
struct template *template_dup(struct template *t);

struct template *template_load(const char *filename);

#endif

/*
Local variables:
c-file-style: "linux"
End:
*/
