/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: template.c
 *
 * Description: interface to debconf templates
 *
 * $Id: template.c,v 1.4 2000/12/09 04:22:30 tausq Exp $
 *
 * cdebconf is (c) 2000 Randolph Chung and others under the following
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
				t->next = tlist;
				tlist = t;
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
				if (buf[1] == '.')
					strvacat(extdesc, sizeof(extdesc), "\n\n", 0);
				else
					strvacat(extdesc, sizeof(extdesc), buf+1, "\n",  0);
			}
			if (*extdesc != 0)
				t->extended_description = strdup(extdesc);

			if (t != 0)
			{
				t->next = tlist;
				tlist = t;
				t = 0;
			}
		}
	}
	fclose(fp);
	return tlist;
}
