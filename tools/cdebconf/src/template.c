#include "common.h"
#include "template.h"
#include "strutl.h"

#include <stdio.h>

struct template *template_new(const char *tag)
{
	struct template *t = NEW(struct template);
	memset(t, 0, sizeof(struct template));
	t->ref = 1;
	t->tag = STRDUP(tag);

	return t;
}

void template_delete(struct template *t)
{
	DELETE(t->tag);
	DELETE(t->type);
	DELETE(t->defaultval);
	DELETE(t->choices);
	DELETE(t->description);
	DELETE(t->extended_description);
	DELETE(t);
}

void template_ref(struct template *t)
{
	t->ref++;
}

void template_deref(struct template *t)
{
	if (--t->ref == 0)
		template_delete(t);
}

struct template *template_load(const char *filename)
{
	char buf[2048], extdesc[8192];
	char *p;
	FILE *fp;
	struct template *tlist = NULL, *t = 0;

	if ((fp = fopen(filename, "r")) == NULL)
		return NULL;
	while (fgets(buf, sizeof(buf), fp))
	{
		p = strstrip(buf);
		if (*p == 0)
		{
			if (t != 0)
			{
				if (tlist == 0)
				{
					tlist = t;
				}
				else
				{
					t->next = tlist;
					tlist = t;
				}
				t = 0;
			}
		}
		if (strstr(p, "Template: ") == p)
			t = template_new(p+10);
		else if (strstr(p, "Type: ") == p && t != 0)
			t->type = strdup(p+6);
		else if (strstr(p, "Default: ") == p && t != 0)
			t->defaultval = strdup(p+9);
		else if (strstr(p, "Choices: ") == p && t != 0)
			t->choices = strdup(p+9);
		else if (strstr(p, "Description: ") == p && t != 0)
		{
			t->description = strdup(p+13);
			extdesc[0] = 0;
			while (fgets(buf, sizeof(buf), fp))
			{
				if (buf[0] != ' ') break;
				strstrip(buf);
				strvacat(extdesc, sizeof(extdesc), buf+1, " ",  0);
			}
			if (*extdesc != 0)
				t->extended_description = strdup(extdesc);
		}
	}
	fclose(fp);
	return tlist;
}
