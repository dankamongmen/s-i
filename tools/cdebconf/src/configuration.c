/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: configuration.c
 *
 * Description: configuration file parsing and access routines
 *              (adapted from APT's Configuration class)
 *
 * $Id: configuration.c,v 1.8 2002/08/07 16:38:44 tfheen Exp $
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
#include "configuration.h"
#include "strutl.h"

#include <stdio.h>
#include <ctype.h>

#define DELIMITER	"::"

/* private functions */
/*
 * Function: config_lookuphlp
 * Input: struct configitem *head - where to start looking
 *        const char *tag - tag to look for
 *        unsigned long len - length of tag
 *        int create - create a new node if it doesn't exist?
 * Output: struct configitem * - node of the item being sought, NULL if not
 *         found
 * Description: helper function for lookups
 * Assumptions: none
 */
static struct configitem *config_lookuphlp(struct configitem *head, 
	const char *tag, unsigned long len, int create)
{
	int res = 1;
	struct configitem *i = head->child;
	struct configitem **last = &head->child;
	struct configitem *newitem;

	/* empty strings match nothing; they are used for lists */
	if (len != 0)
	{
		for (; i != 0; last = &i->next, i = i->next)
			if ((res = strcountcmp(i->tag, i->tag+strlen(i->tag), tag, tag + len)) == 0)
				break;
	}
	else
		for (; i != 0; last = &i->next, i = i->next);

	if (res == 0)
		return i;
	if (create == 0)
		return 0;

	newitem = NEW(struct configitem);
	newitem->tag = malloc(len+1);
	newitem->tag[len] = 0;
	memcpy(newitem->tag, tag, len);
	newitem->value = 0;
	newitem->next = *last;
	newitem->parent = head;
	newitem->child = 0;
	*last = newitem;
	return newitem;
}

/*
 * Function: config_lookup
 * Input: struct configuration *config - config object to look inside
 *        const char *tag - tag of item to look for
 *        int create - create if doesn't exist?
 * Output: struct configitem * - item being sough, NULL if doesn't exist
 * Description: lookup a given configuration item
 * Assumptions: none
 */
static struct configitem *config_lookup(struct configuration *config, 
	const char *tag, int create)
{
	const char *start, *end, *tagend;
	struct configitem *item = config->root;

	if (tag == 0)
		return config->root->child;

	start = tag;
	end = start + strlen(tag);
	tagend = tag;

	for (; end - tagend >= 2; tagend++)
	{
		if (tagend[0] == ':' && tagend[1] == ':')
		{
			item = config_lookuphlp(item, start, tagend-start, create);
			if (item == 0)
				return 0;
			tagend = start = tagend + 2;
		}
	}

	/* trailing :: */
	if (end - start == 0)
		if (create == 0) return 0;
	
	item = config_lookuphlp(item, start, end-start, create);
	return item;
}

/*
 * Function: config_fulltag
 * Input: const struct configitem *top - head of config tree
 *        const struct configitem *stop - when to stop
 *        char *tag - buffer to build the tag
 *        const size_t maxlen - length of buffer
 * Output: none
 * Description: recursive builds the complete tag name, seperated by :: 
 * Assumptions: none
 */
void config_fulltag(const struct configitem *top, 
	const struct configitem *stop, char *tag, const size_t maxlen)
{
	char buf[maxlen];
	buf[0] = 0;

	if (top->parent == 0 || top->parent->parent == 0 ||
		top->parent == stop)
	{
		strncpy(tag, top->tag, maxlen);
		return;
	}
	config_fulltag(top->parent, stop, buf, sizeof(buf));
	strvacat(tag, maxlen, buf, DELIMITER, top->tag, 0);
}

/* public functions */
/*
 * Function: config_get
 * Input: struct configurat *cfg - configuration object
 *        const char *tag - tag of configuration item to retrieve
 *        const char *defaultvalue - value to return if tag not found
 * Output: const char * - value of tag (or default)
 * Description: retrieves in string format the value of a given parameter
 * Assumptions: none
 */
static const char *config_get(struct configuration *cfg, const char *tag, 
		const char *defaultvalue)
{
	struct configitem *item = config_lookup(cfg, tag, 0);
	if (item == 0) 
		return defaultvalue;
	else
		return item->value;
}

/*
 * Function: config_geti
 * Input: struct configurat *cfg - configuration object
 *        const char *tag - tag of configuration item to retrieve
 *        const char *defaultvalue - value to return if tag not found
 * Output: int - value of tag (or default)
 * Description: retrieves in integer format the value of a given parameter
 * Assumptions: if the parameter being sought exists but is a string, 
 *              return defaultvalue
 */
static int config_geti(struct configuration *cfg, const char *tag, 
		int defaultvalue)
{
	int res;
	char *end;
	struct configitem *item = config_lookup(cfg, tag, 0);
	if (item == 0) 
		return defaultvalue;

	res = strtol(item->value, &end, 0);
	if (end == item->value)
		return defaultvalue;
	else
		return res;
}

/*
 * Function: config_set
 * Input: struct configuration *cfg - configuration object
 *        const char *tag - tag of configuration item
 *        const char *value - value of configuration item
 * Output: none
 * Description: Sets the value of a given (string-type) configuration 
 *              parameter
 * Assumptions: none
 */
static void config_set(struct configuration *cfg, const char *tag,
		const char *value)
{
	struct configitem *item = config_lookup(cfg, tag, 1);
	if (item == 0) return;
	DELETE(item->value);
	item->value = STRDUP(value);
}

/*
 * Function: config_seti
 * Input: struct configuration *cfg - configuration object
 *        const char *tag - tag of configuration item
 *        const char *value - value of configuration item
 * Output: none
 * Description: Sets the value of a given (integer-type) configuration 
 *              parameter
 * Assumptions: none
 */
static void config_seti(struct configuration *cfg, const char *tag,
		int value)
{
	char s[50];
	snprintf(s, sizeof(s), "%i", value);
	config_set(cfg, tag, s);
}

/*
 * Function: config_exists
 * Input: struct configuration *cfg - configuration object
 *        const char *tag - tag of configuration parmaeter being sought
 * Output: int - 0 if exists, non-zero otherwise
 * Description: determines if a configuration parameter with the given
 *              tag exists
 * Assumptions: none
 */
static int config_exists(struct configuration *cfg, const char *tag)
{
	struct configitem *item = config_lookup(cfg, tag, 0);
	return (item != 0);
}

/*
 * Function: config_read
 * Input: struct configuration *cfg - configuration object
 *        const char *filename - name of file to parse
 * Output: int - 1 if suceeded, 0 otherwise
 * Description: parses a BIND-like configuration file for configuration
 *              parameters
 * Assumptions: const buffer sizes below limits the length of tags
 */
static int config_read(struct configuration *cfg, const char *filename)
{
	FILE *infp;
	char parenttag[256];
	char tag[256], value[4096], item[4096];
	char buffer[4096];
	char *buf;
	char linebuf[8192];
	char *stack[100];
	char *s, *q, *start, *stop;
	char termchar;
	unsigned int stackpos = 0;
	int curline = 0, i;
	int incomment = 0, inquote = 0;

	linebuf[0] = 0;
	parenttag[0] = 0;
	for (i = 0; i < DIM(stack); i++)
		stack[i] = NULL;

	if ((infp = fopen(filename, "r")) == NULL)
		return 0;

	while (fgets(buffer, sizeof(buffer), infp))
	{
		curline++;
		buf = strstrip(buffer);

		/* multiline comments -- check for end-of-comment */
		if (incomment != 0)
		{
			for (s = buf; *s != 0; s++)
			{
				if (s[0] == '*' && s[1] == '/')
				{
					memmove(buf, s+2, strlen(s+2)+1);
					incomment = 0;
					break;
				}
			}
			if (incomment != 0)
				continue;
		}

		/* discard single line comments */
		inquote = 0;
		for (s = buf; *s != 0; s++)
		{
			if (*s == '"')
				inquote = !inquote;
			if (inquote != 0)
				continue;

			if (s[0] == '/' && s[1] == '/')
			{
				*s = 0;
				break;
			}
		}

		/* look for multiline comments */
		inquote = 0;
		for (s = buf; *s != 0; s++)
		{
			if (*s == '"')
				inquote = !inquote;
			if (inquote != 0)
				continue;
			if (s[0] == '/' && s[1] == '*')
			{
				incomment = 1;
				for (q = buf; *q != 0; q++)
				{
					if (q[0] == '*' && q[1] == '/')
					{
						memmove(s, q+2, strlen(q+2)+1);
						incomment = 0;
						break;
					}
				}

				if (incomment != 0)
				{
					*s = 0;
					break;
				}
			}
		}

		/* skip blank lines */
		if (buf[0] == 0) 
			continue;
		
		/* valid line */
		buf = strstrip(buf);
		inquote = 0;
		for (s = buf; *s != 0;)
		{
			if (*s == '"')
				inquote = !inquote;

			if (inquote == 0 && (*s == '{' || *s == ';' || *s == '}'))
			{
				/* add the last fragment to the buffer */
				start = buf;
				stop = s;
				for (; start != s && isspace(*start) != 0; start++);
				for (; stop != start && isspace(stop[-1]) != 0; stop--);
				if (linebuf[0] != 0 && stop - start != 0)
					strcat(linebuf, " ");
				strncat(linebuf, start, stop-start);

				/* remove the fragment */
				termchar = *s;
				memmove(buf, s+1, strlen(s+1)+1);
				s = buf;

				/* syntax error */
				if (termchar == '{' && linebuf[0] == 0)
				{
					INFO(INFO_ERROR, "Syntax error %s:%u: block starts with no name\n", filename, curline);
					return 0;
				}

				if (linebuf[0] == 0)
				{
					if (termchar == '}')
					{
						if (stackpos == 0)
							parenttag[0] = 0;
						else
							snprintf(parenttag, sizeof(parenttag), stack[--stackpos]);
					}
					continue;
				}

				/* parse off the tag */
				q = (char *)linebuf;
				tag[0] = 0;
				if (strparsequoteword(&q, tag, sizeof(tag)) == 0)
				{
					INFO(INFO_ERROR, "Syntax error %s:%u: Malformed tag\n", filename, curline);
					return 0;
				}

				/* parse off the value */
				value[0] = 0;
				if (strparsecword(&q, value, sizeof(value)) == 0
				    && strparsequoteword(&q, value, sizeof(value)) == 0)
				{
					if (termchar != '{')
					{
						strncpy(value, tag, sizeof(value));
						tag[0] = 0;
					}
				}

				/* extra junk */
				if (strlen(q) != 0)
				{
					INFO(INFO_ERROR, "Syntax error %s:%u: Extra junk after tag", filename, curline);
					return 0;
				}

				if (termchar == '{')
				{
                                        /* 99, not 100.  100 gives possible
                                           off-by-one error */
					if (stackpos <= 99)
						stack[stackpos++] = strdup(parenttag);
					if (value[0] != 0)
					{
						strvacat(tag, sizeof(tag),
							DELIMITER, value, 0);
						value[0] = 0;
					}

					if (parenttag[0] == 0)
						strncpy(parenttag, tag, 
							sizeof(parenttag));
					else
						strvacat(parenttag,
							sizeof(parenttag), 
							DELIMITER, tag, 0);
					tag[0] = 0;
				}

				/* generate the item name */
				if (parenttag[0] == 0)
					strncpy(item, tag, sizeof(item));
				else
				{
					if (termchar != '{' || tag[0] != 0)
					{
						item[0] = 0;
						strvacat(item, sizeof(item),
							parenttag, DELIMITER,
							tag, 0);
					}
					else
					{
						strncpy(item, parenttag, 
							sizeof(item));
					}
				}
				
				/* insert the data item into the tree */
				config_set(cfg, item, value);

				linebuf[0] = 0;

				if (termchar == '}')
				{
					if (stackpos == 0)
						parenttag[0] = 0;
					else
					{
						strncpy(parenttag,
							stack[--stackpos],
							sizeof(parenttag));
						DELETE(stack[stackpos]);
					}
				}
			}
			else
				s++;
		}
		
		/* store the line fragment */
		strstrip(buf);
		if (buf[0] != 0 && linebuf[0] != 0)
			strvacat(linebuf, sizeof(linebuf), " ", 0);
		strcat(linebuf, buf);
	}
	fclose(infp);
	return 1;
}

/*
 * Function: config_dump
 * Input: struct configuration *cfg - configuration object
 * Output: none
 * Description: prints out all the configuration items currently stored in the
 *              object
 * Assumptions: none
 */
static void config_dump(struct configuration *cfg)
{
	const struct configitem *top = cfg->tree(cfg, 0);
	char buf[512];

	while (top != 0)
	{
		buf[0] = 0;
		config_fulltag(top, 0, buf, sizeof(buf));
		printf("%s \"%s\"\n", buf, top->value);

		if (top->child != 0)
		{
			top = top->child;
			continue;
		}
		while (top != 0 && top->next == 0)
			top = top->parent;
		if (top != 0)
			top = top->next;
	}
}

/*
 * Function: config_tree
 * Input: struct configuration *cfg - configuration object
 *        const char *tag - tag to look for
 * Output: struct configitem * - head of tree at the point where the tag
 *                               is as given
 * Description: returns an internal pointer to a tree structure representing
 *              a node with the given tag
 * Assumptions: external callers should not change the structure of the
 *              tree returned
 */
static struct configitem *config_tree(struct configuration *cfg, const char *tag)
{
	return config_lookup(cfg, tag, 0);
}

/*
 * Function: config_new
 * Input: none
 * Output: struct configuration * - configuration object created
 * Description: Creates a configuration object
 * Assumptions: NEW succeeds
 */
struct configuration *config_new(void)
{
	struct configuration *config = NEW(struct configuration);
	config->root = NEW(struct configitem);
	memset(config->root, 0, sizeof(struct configitem));
	config->get = config_get;
	config->geti = config_geti;
	config->set = config_set;
	config->seti = config_seti;
	config->exists = config_exists;
	config->read = config_read;
	config->dump = config_dump;
	config->tree = config_tree;

	return config;
}

/*
 * Function: config_delete
 * Input: struct configuration *cfg - configuration object
 * Output: none
 * Description: releases the memory used by cfg 
 * Assumptions: config != NULL
 */
void config_delete(struct configuration *config)
{
	struct configitem *next;
	struct configitem *top = config->root;

	while (top != 0)
	{
		if (top->child != 0)
		{
			top = top->child;
			continue;
		}

		while (top != 0 && top->next == 0)
		{
			next = top->parent;
			DELETE(top);
			top = next;
		}

		if (top != 0)
		{
			next = top->next;
			DELETE(top);
			top = next;
		}
	}
}
