/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: strutl.c
 *
 * Description: misc. routines for string handling
 *
 * $Id: strutl.c,v 1.15 2002/07/01 06:58:37 tausq Exp $
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
#include "strutl.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

int strcountcmp(const char *s1, const char *e1, const char *s2, const char *e2)
{
	for (; s1 != e1 && s2 != e2 && *s1 == *s2; s1++, s2++) ;

	if (s1 == e1 && s2 == e2)
		return 0;

	if (s1 == e1)
		return 1;
	if (s2 == e2)
		return -1;
	if (*s1 < *s2)
		return -1;
	return 1;
}

/*
 * Strips leading and trailing spaces from a string; also strips trialing
 * newline characters from the string
 */
char *strstrip(char *buf)
{
	char *end;

	/* skip initial spaces */
	for (; *buf != 0 && isspace(*buf); buf++) ;

	if (*buf == 0)
		return buf;

	end = buf + strlen(buf) - 1;
	while (end != buf - 1)
	{
		if (isspace(*end) == 0)
			break;

		*end = 0;
		end--;
	}
	return buf;
}

char *strlower(char *buf)
{
	char *p = buf;
	while (*p != 0) *p = tolower(*p);
	return buf;
}

/* 
 * concatenates arbitrary number of strings to a buffer, with bounds
 * checking
 */
void strvacat(char *buf, size_t maxlen, ...)
{
	va_list ap;
	char *str;
	size_t len = strlen(buf);

	va_start(ap, maxlen);
	
	while (1)
	{
		str = va_arg(ap, char *);
		if (str == NULL)
			break;
		if (len + strlen(str) > maxlen)
			break;
		strcat(buf, str);
		len += strlen(str);
	}
	va_end(ap);
}

int strparsecword(char **inbuf, char *outbuf, size_t maxlen)
{
	char buffer[maxlen];
	char *buf = buffer;
	char *c = *inbuf;

	for (; *c != 0 && isspace(*c); c++);

	if (*c == 0) 
		return 0;
	if (strlen(*inbuf) > maxlen)
		return 0;
	for (; *c != 0; c++)
	{
		if (*c == '"')
		{
			for (; *c != 0 && *c != '"'; c++)
				*buf++ = *c;
			if (*c == 0)
				return 0;
			continue;
		}
		
		if (c != *inbuf && isspace(*c) != 0 && isspace(c[-1]) != 0)
			continue;
		if (isspace(*c) == 0)
			return 0;
		*buf++ = ' ';
	}
	*buf = 0;
	strncpy(outbuf, buffer, maxlen);
	*inbuf = c;

	return 1;
}

int strparsequoteword(char **inbuf, char *outbuf, size_t maxlen)
{
	char buffer[maxlen];
	char tmp[3];
	char *start, *i;
	char *c = *inbuf;

    /* skip ws, return if empty */
	for (; *c != 0 && isspace(*c); c++); 
	if (*c == 0)
		return 0;

    start = c;
	
	for (; *c != 0 && isspace(*c) == 0; c++)
	{
		if (*c == '"')
		{
			for (c++; *c != 0 && *c != '"'; c++);
			if (*c == 0)
				return 0;
		}
		if (*c == '[')
		{
			for (c++; *c != 0 && *c != ']'; c++);
			if (*c == 0)
				return 0;
		}
	}

	/* dequote the string */
	for (i = buffer; i < buffer + maxlen && start < c; i++)
	{
		if (*start == '%' && start + 2 < c)
		{
			tmp[0] = start[1];
			tmp[1] = start[2];
			tmp[2] = 0;
			*i = (char)strtol(tmp, 0, 16);
			start += 3;
			continue;
		}
		if (*start != '"')
			*i = *start;
		else
			i--;
		start++;
	}
	*i = 0;
	
	strncpy(outbuf, buffer, maxlen);

    /* skip trailing spaces */
	for (; *c != 0 && isspace(*c); c++);
	*inbuf = c;

	return 1;
}

int strchoicesplit(const char *inbuf, char **argv, size_t maxnarg)
{
	int argc = 0;
	const char *s = inbuf, *e;
	char *p;

	if (inbuf == 0) return 0;

	INFO(INFO_VERBOSE, "Splitting [%s]\n", inbuf);
	while (*s != 0 && argc < maxnarg)
	{
		/* skip initial spaces */
		while (isspace(*s)) s++;

		/* find end */
		e = s;
		while (*e != 0 && *e != ',') e++;

		argv[argc] = malloc(e-s+1);
		memcpy(argv[argc], s, e-s);
		argv[argc][e-s] = 0;
		p = &argv[argc][e-s-1];
		/* strip off trailing spaces */
		while (p > argv[argc] && *p == ' ') *p-- = 0;
		argc++;

		s = e;
		if (*s == ',') s++;
	}
	return argc;
}

int strcmdsplit(char *inbuf, char **argv, size_t maxnarg)
{
	int argc = 0;
	int inspace = 1;

	if (maxnarg == 0) return 0;
	
	for (; *inbuf != 0; inbuf++)
	{
		if (isspace(*inbuf) != 0)
		{
			inspace = 1;
			*inbuf = 0;
		}
		else if (inspace != 0)
		{
			inspace = 0;
			argv[argc++] = inbuf;
			if (argc >= maxnarg)
				break;
		}
	}

	return argc;
}

void strunescape(const char *inbuf, char *outbuf, const size_t maxlen)
{
	const char *p = inbuf;
	int i = 0;
	char tmp[3];
	while (*p != 0 && i < maxlen-1)
	{
		if (*p == '%')
		{
			if (i + 4 >= maxlen) break;
			tmp[0] = *(p+1);
			tmp[1] = *(p+2);
			tmp[2] = 0;
			outbuf[i++] = atoi(tmp);
			p += 3;
		}
		else
		{
			outbuf[i++] = *p++;
		}
	}
	outbuf[i] = 0;
}

void strescape(const char *inbuf, char *outbuf, const size_t maxlen)
{
	const char *p = inbuf;
	int i = 0;
	while (*p != 0 && i < maxlen-1)
	{
		if (*p == '%' || *p == '"' || *p == '\r' || *p == '\n')
		{
			if (i + 4 >= maxlen) break;
			outbuf[i] = '%';
			sprintf(&outbuf[i+1], "%02X", (unsigned int)*p);
			p++;
			i += 3;
		}
		else
			outbuf[i++] = *p++;
	}
	outbuf[i] = 0;
}

int strwrap(const char *str, const int width, char *lines[], int maxlines)
{
	/* "Simple" greedy line-wrapper */
	int len = STRLEN(str);
	int l = 0;
	const char *s, *e, *end, *lb;

	if (str == 0) return 0;

	/*
	 * s = first character of current line, 
	 * e = last character of current line
	 * end = last character of the input string (the trailing \0)
	 */
	s = e = str;
	end = str + len;
	
	while (len > 0)
	{
		/* try to fit the most characters */
		e = s + width - 1;
		
		/* simple case: we got everything */
		if (e >= end) 
		{
			e = end;
		}
		else
		{
			/* find a breaking point */
			while (e > s && !isspace(*e) && *e != '.' && *e != ',')
				e--;
		}
		/* no word-break point found, so just unconditionally break 
		 * the line */
		if (e == s) e = s + width;

		/* if there's an explicit linebreak, honor it */
		lb = strchr(s, '\n');
		if (lb != NULL && lb < e) e = lb;

		lines[l] = (char *)malloc(e-s+2);
		strncpy(lines[l], s, e-s+1);
		lines[l][e-s+1] = 0;
		CHOMP(lines[l]);

		len -= (e-s+1);
		s = e + 1;
		if (++l >= maxlines) break;
	}
	return l;
}

int strlongest(char **strs, int count)
{
	int i, max = 0;

	for (i = 0; i < count; i++)
	{
		if (strlen(strs[i]) > max)
			max = strlen(strs[i]);
	}
	return max;
}
