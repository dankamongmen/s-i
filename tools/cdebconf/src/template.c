/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: template.c
 *
 * Description: interface to debconf templates
 *
 * $Id: template.c,v 1.13 2002/11/20 23:47:16 barbier Exp $
 *
 * cdebconf is (c) 2000-2001 Randolph Chung and others under the following
 * license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ***********************************************************************/
#include "common.h"
#include "template.h"
#include "strutl.h"

#include <stdio.h>

static const char *template_lget(struct template *t, const char *lang,
                const char *field);
static void template_lset(struct template *t, const char *lang,
                const char *field, const char *value);
static const char *template_next_lang(struct template *t, const char *l);
static const char *getlanguage(void);

const char *template_fields_list[] = {
        "tag",
        "type",
        "default",
        "choices",
        "description",
        "extended_description",
        NULL
};

static const char *getlanguage(void)
{
        /*  NULL is a valid return value  */
	return getenv("DEBCONF_LANG");
}

/*
 * Function: template_new
 * Input: a tag, describing which template this is.  Can be null.
 * Output: a blank template struct.  Tag is strdup-ed, so the original
           string may change without harm.
           The fields structure is also allocated to store English fields
 * Description: allocate a new, empty struct template.
 * Assumptions: NEW succeeds
 * Todo: 
 */

struct template *template_new(const char *tag)
{
	struct template_l10n_fields *f = NEW(struct template_l10n_fields);
	struct template *t = NEW(struct template);
	memset(f, 0, sizeof(struct template_l10n_fields));
	f->language = strdup("C");
	memset(t, 0, sizeof(struct template));
	t->ref = 1;
	t->tag = STRDUP(tag);
	t->lget = template_lget;
	t->lset = template_lset;
	t->next_lang = template_next_lang;
	t->fields = f;
	return t;
}

void template_delete(struct template *t)
{
	struct template_l10n_fields *p, *q;

	DELETE(t->tag);
	DELETE(t->type);
	p = t->fields;
	DELETE(t);
	while (p != NULL)
	{
		q = p->next;
		DELETE(p->defaultval);
		DELETE(p->choices);
		DELETE(p->description);
		DELETE(p->extended_description);
		DELETE(p);
		p = q;
	}
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

/*
 * Function: template_dup
 * Input: a template, being the template we want to duplicate
 * Output: a copy of the template passed as input
 * Description: duplicate a template
 * Assumptions: template_new succeeds, STRDUP succeeds.
 */

struct template *template_dup(struct template *t)
{
        struct template *ret = template_new(t->tag);
        struct template_l10n_fields *from, *to;

        ret->type = STRDUP(t->type);
        if (t->fields == NULL)
                return ret;

        ret->fields = NEW(struct template_l10n_fields);

        from = t->fields;
        to = ret->fields;
        /*  Iterate over available languages  */
        while (1)
        {
                to->defaultval = STRDUP(from->defaultval);
                to->choices = STRDUP(from->choices);
                to->description = STRDUP(from->description);
                to->extended_description = STRDUP(from->extended_description);

                if (from->next == NULL)
                {
                        to->next = NULL;
                        break;
                }
                to->next = NEW(struct template_l10n_fields);
                from = from->next;
                to = to->next;
        }
        return ret;
}

static const char *template_field_get(struct template_l10n_fields *p,
                const char *field)
{
    if (strcasecmp(field, "default") == 0)
        return p->defaultval;
    else if (strcasecmp(field, "choices") == 0)
        return p->choices;
    else if (strcasecmp(field, "description") == 0)
        return p->description;
    else if (strcasecmp(field, "extended_description") == 0)
        return p->extended_description;
    return NULL;
}

static void template_field_set(struct template_l10n_fields *p,
                const char *field, const char *value)
{
    if (strcasecmp(field, "default") == 0)
    {
        DELETE(p->defaultval);
        p->defaultval = STRDUP(value);
    }
    else if (strcasecmp(field, "choices") == 0)
    {
        DELETE(p->choices);
        p->choices = STRDUP(value);
    }
    else if (strcasecmp(field, "description") == 0)
    {
        DELETE(p->description);
        p->description = STRDUP(value);
    }
    else if (strcasecmp(field, "extended_description") == 0)
    {
        DELETE(p->extended_description);
        p->extended_description = STRDUP(value);
    }
}

/*
 * Function: template_lget
 * Input: a template
 * Input: a language name
 * Input: a field name
 * Output: the value of the given field in the given language, field
 *         name may be any of type, default, choices, description and
 *         extended_description
 * Description: get field value
 * Assumptions: 
 */

static const char *template_lget(struct template *t, const char *lang,
                const char *field)
{
    struct template_l10n_fields *p;
    const char *ret = NULL, *altret = NULL;
    char *orig_field;
    char *altlang;
    const char *curlang;

    if (strcasecmp(field, "tag") == 0)
        return t->tag;
    else if (strcasecmp(field, "type") == 0)
        return t->type;

    /*   If field is Foo-xx.UTF-8 then call template_lget(t, "xx", "Foo")  */
    if (strchr(field, '-') != NULL)
    {
        orig_field = strdup(field);
        altlang = strchr(orig_field, '-');
        *altlang = 0;
        altlang++;
        if (strstr(altlang, ".UTF-8") == altlang + 2)
        {
            *(altlang+2) = 0;
            ret = template_lget(t, altlang, orig_field);
        }
        else if (strstr(altlang, ".UTF-8") == altlang + 5)
        {
            *(altlang+5) = 0;
            ret = template_lget(t, altlang, orig_field);
        }
#ifndef NODEBUG
        else
            fprintf(stderr, "Unknown localized field:\n%s\n", field);
#endif
        free(orig_field);
        return ret;
    }

    if (lang != NULL && *lang == 0)
        curlang = getlanguage();
    else
        curlang = lang;

    p = t->fields;
    while (p != NULL)
    {
        /*  Exact match  */
        if (curlang == NULL || strcmp(p->language, curlang) == 0)
        {
            ret = template_field_get(p, field);
            if (ret == NULL)
                ret = template_field_get(t->fields, field);
            return ret;
        }

        /*  Language is xx_XX and a xx field is found  */
        if (strlen(p->language) == 2 && strncmp(curlang, p->language, 2) == 0)
            altret = template_field_get(p, field);

        p = p->next;
    }
    if (altret != NULL)
        return altret;
    /*  Default value  */
    return template_field_get(t->fields, field);
}

static void template_lset(struct template *t, const char *lang,
                const char *field, const char *value)
{
    struct template_l10n_fields *p, *last;
    char *orig_field;
    char *altlang;
    const char *curlang;

    if (strcasecmp(field, "tag") == 0)
    {
        t->tag = STRDUP(value);
        return;
    }
    else if (strcasecmp(field, "type") == 0)
    {
        t->type = STRDUP(value);
        return;
    }

    /*   If field is Foo-xx.UTF-8 then call template_lget(t, "xx", "Foo")  */
    if (strchr(field, '-') != NULL)
    {
        orig_field = strdup(field);
        altlang = strchr(orig_field, '-');
        *altlang = 0;
        altlang++;
        if (strstr(altlang, ".UTF-8") == altlang + 2)
        {
            *(altlang+2) = 0;
            template_lset(t, altlang, orig_field, value);
        }
        else if (strstr(altlang, ".UTF-8") == altlang + 5)
        {
            *(altlang+5) = 0;
            template_lset(t, altlang, orig_field, value);
        }
#ifndef NODEBUG
        else
            fprintf(stderr, "Unknown localized field:\n%s\n", field);
#endif
        free(orig_field);
        return;
    }

    if (lang != NULL && *lang == 0)
        curlang = getlanguage();
    else
        curlang = lang;

    p = t->fields;
    last = p;
    while (p != NULL)
    {
        if (curlang == NULL || strcmp(p->language, curlang) == 0)
        {
            template_field_set(p, field, value);
            return;
        }
        last = p;
        p = p->next;
    }
    p = NEW(struct template_l10n_fields);
    memset(p, 0, sizeof(struct template_l10n_fields));
    p->language = STRDUP(curlang);
    last->next = p;
    template_field_set(p, field, value);
}

static const char *template_next_lang(struct template *t, const char *lang)
{
    struct template_l10n_fields *p;

    p = t->fields;
    while (p != NULL)
    {
        if (lang == NULL || strcmp(p->language, lang) == 0)
        {
            if (p->next == NULL)
                return NULL;
            return p->next->language;
        }
        p = p->next;
    }
    return NULL;
}

struct template *template_load(const char *filename)
{
	char buf[2048], extdesc[8192];
	char *lang;
	char *p, *bufp;
	FILE *fp;
	struct template *tlist = NULL, *t = 0;
	unsigned int i;
	
	if ((fp = fopen(filename, "r")) == NULL)
		return NULL;
	while (fgets(buf, sizeof(buf), fp))
	{
		lang = NULL;
		p = strstrip(buf);
		if (*p == 0)
		{
			if (t != 0)
			{
				t->next = tlist;
				tlist = t;
				t = 0;
			}
		}
		if (strstr(p, "Template: ") == p)
			t = template_new(p+10);
		else if (strstr(p, "Type: ") == p && t != 0)
			template_lset(t, NULL, "type", p+6);
		else if (strstr(p, "Default: ") == p && t != 0)
			template_lset(t, NULL, "default", p+9);
		else if (strstr(p, "Choices: ") == p && t != 0)
			template_lset(t, NULL, "choices", p+9);
		else if (strstr(p, "Choices-") == p && t != 0) 
		{
			if (strstr(p, ".UTF-8: ") == p + 10)
			{
				lang = strndup(p+8, 2);
				template_lset(t, lang, "choices", p+18);
			}
			else if (strstr(p, ".UTF-8: ") == p + 13)
			{
				lang = strndup(p+8, 5);
				template_lset(t, lang, "choices", p+21);
			}
			else
			{
#ifndef NODEBUG
				fprintf(stderr, "Unknown localized field:\n%s\n", p);
#endif
                                continue;
			}
		}
		else if (strstr(p, "Description: ") == p && t != 0)
		{
			template_lset(t, NULL, "description", p+13);
			extdesc[0] = 0;
			i = fgetc(fp);
			/* Don't use fgets unless you _need_ to, a
			   translated description may follow and it
			   will get lost if you just fgets without
			   thinking about what you are doing. :)

			   -- tfheen, 2001-11-21
			*/
                        while (i == ' ')
			{
                                if (i != ungetc(i, fp)) { /* ERROR */ }
                                fgets(buf, sizeof(buf), fp);
                                strvacat(extdesc, sizeof(extdesc), buf+1, 0);
                                i = fgetc(fp);
                                
			}
                        ungetc(i, fp); /* toss the last one back */
			if (*extdesc != 0)
			{

				/* remove extraneous linebreaks */
				/* remove internal linebreaks unless the next
				 * line starts with a space; also change
				 * lines that contain a . only to an empty
				 * line
				 */
				for (bufp = extdesc; *bufp != 0; bufp++)
					if (*bufp == '\n')
					{
						if (*(bufp+1) == '.' &&
							*(bufp+2) == '\n')
						{
							*(bufp+1) = ' ';
							bufp+=2;
						}
						else if (*(bufp+1) != ' ')
						{
							if (*(bufp+1) != 0)
								*bufp = ' ';
                                                        else
								*bufp = 0;
						}
					}
					
				template_lset(t, NULL, "extended_description", extdesc);
			}
		}
		else if (strstr(p, "Description-") == p && t != 0)
		{
			if (strstr(p, ".UTF-8: ") == p + 14)
			{
				lang = strndup(p+12, 2);
				template_lset(t, lang, "description", p+22);
			}
			else if (strstr(p, ".UTF-8: ") == p + 17)
			{
				lang = strndup(p+12, 5);
				template_lset(t, lang, "description", p+25);
			}
			else
			{
#ifndef NODEBUG
				fprintf(stderr, "Unknown localized field:\n%s\n", p);
#endif
				/*  Skip extended description  */
				lang = NULL;
			}
			extdesc[0] = 0;
			i = fgetc(fp);
			/* Don't use fgets unless you _need_ to, a
			   translated description may follow and it
			   will get lost if you just fgets without
			   thinking about what you are doing. :)

			   -- tfheen, 2001-11-21
			*/
			while (i == ' ') 
			{
				if (i != ungetc(i, fp)) { /* ERROR */ }
				fgets(buf, sizeof(buf), fp);
				strvacat(extdesc, sizeof(extdesc), buf+1, 0);
                                i = fgetc(fp);
			}
                        ungetc(i, fp); /* toss the last one back */
			if (*extdesc != 0)
			{
				/* remove extraneous linebreaks */
				/* remove internal linebreaks unless the next
				 * line starts with a space; also change
				 * lines that contain a . only to an empty
				 * line
				 */
				for (bufp = extdesc; *bufp != 0; bufp++)
					if (*bufp == '\n')
					{
						if (*(bufp+1) == '.' &&
						    *(bufp+2) == '\n')
						{
							*(bufp+1) = ' ';
							bufp+=2;
						}
						else if (*(bufp+1) != ' ')
						{
							if (*(bufp+1) != 0)
								*bufp = ' ';
                                                        else
								*bufp = 0;
						}
					}
				
				if (lang)
					template_lset(t, lang, "extended_description", extdesc);
			}
		}
		if (lang)
			free(lang);
	}

	if (t != 0)
	{
		t->next = tlist;
		tlist = t;
		t = 0;
	}

	fclose(fp);
	t = tlist;
	return tlist;
}
/*
Local variables:
c-file-style: "linux"
End:
*/
