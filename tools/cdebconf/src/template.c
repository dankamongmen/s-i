/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: template.c
 *
 * Description: interface to debconf templates
 *
 * $Id: template.c,v 1.7 2002/05/18 22:35:05 tfheen Rel $
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

/*
 * Function: template_new
 * Input: a tag, describing which template this is.  Can be null.
 * Output: a blank template struct.  Tag is strdup-ed, so the original
           string may change without harm.
 * Description: allocate a new, empty struct template.
 * Assumptions: NEW succeeds
 * Todo: 
 */

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

/*
 * Function: template_dup
 * Input: a template, being the template we want to duplicate
 * Output: a copy of the template passed as input
 * Description: duplicate a template
 * Assumptions: template_new succeeds, STRDUP succeeds.
 * Todo: Handle localization
 */

struct template *template_dup(struct template *t)
{
        struct template *ret = template_new(t->tag);
        ret->type = STRDUP(t->type);
        ret->defaultval = STRDUP(t->defaultval);
        ret->choices = STRDUP(t->choices);
        ret->description = STRDUP(t->description);
        ret->extended_description = STRDUP(t->extended_description);
        return ret;
}

struct template *template_load(const char *filename)
{
	char buf[2048], extdesc[8192];
	char *p, *bufp;
	FILE *fp;
	struct template *tlist = NULL, *t = 0;
	unsigned int i;
	
	if ((fp = fopen(filename, "r")) == NULL)
		return NULL;
	while (fgets(buf, sizeof(buf), fp))
	{
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
		{
			t = template_new(p+10);
		}
		else if (strstr(p, "Type: ") == p && t != 0)
			t->type = strdup(p+6);
		else if (strstr(p, "Default: ") == p && t != 0)
			t->defaultval = strdup(p+9);
		else if (strstr(p, "Choices: ") == p && t != 0)
			t->choices = strdup(p+9);
		else if (strstr(p, "Choices-") == p && t != 0) 
		{
			struct language_description *langdesc = malloc(sizeof(struct language_description));
			struct language_description *lng_tmp = 0;
			memset(langdesc,0,sizeof(struct language_description));
			/* We assume that the language codes are
			   always 2 characters long, */

			langdesc->language = malloc(3);
			snprintf(langdesc->language,3,"%.2s",p+8);
			
			langdesc->choices = strdup(p+11);

			if (t->localized_descriptions == NULL) 
			{
				t->localized_descriptions = langdesc;
			}
			else
			{
				lng_tmp = t->localized_descriptions;
				while (lng_tmp != NULL)
				{
					if (strcmp(lng_tmp->language,langdesc->language) == 0)
					{
						if (lng_tmp->choices)
							free(lng_tmp->choices);
						lng_tmp->choices = langdesc->choices;
						free(langdesc->language);
						free(langdesc);
						langdesc = NULL;
						break;
					}
					lng_tmp = lng_tmp->next;
				}
				if (langdesc != NULL) 
				{
					langdesc->next = t->localized_descriptions;
					t->localized_descriptions = langdesc;
				}
			}
		}
		else if (strstr(p, "Description: ") == p && t != 0)
		{
			t->description = strdup(p+13);
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
							*bufp = ' ';
					}
					
				t->extended_description = strdup(extdesc);
			}
		}
		else if (strstr(p, "Description-") == p && t != 0)
		{
			struct language_description *langdesc = malloc(sizeof(struct language_description));
			struct language_description *lng_tmp = 0;
			memset(langdesc,0,sizeof(struct language_description));

			/* We assume that the language codes are
			   always 2 characters long, */

			langdesc->language = malloc(3);
			snprintf(langdesc->language,3,"%.2s",p+12);
			langdesc->description = strdup(p+16);

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
							*bufp = ' ';
					}
				
				langdesc->extended_description = strdup(extdesc);
			}
			if (t->localized_descriptions == NULL) 
			{
				t->localized_descriptions = langdesc;
			}
			else
			{
				lng_tmp = t->localized_descriptions;
				while (lng_tmp != NULL)
				{
					if (strcmp(lng_tmp->language,langdesc->language) == 0)
					{
						if (lng_tmp->description != NULL)
							free(lng_tmp->description);
						if (lng_tmp->extended_description != NULL)
							free(lng_tmp->extended_description);
						lng_tmp->description = langdesc->description;
						lng_tmp->extended_description = langdesc->extended_description;
						free(langdesc->language);
						free(langdesc);
						langdesc = NULL;
						break;
					}
					lng_tmp = lng_tmp->next;
				}
				if (langdesc != NULL) 
				{
					langdesc->next = t->localized_descriptions;
					t->localized_descriptions = langdesc;
				}
			}
		}
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
