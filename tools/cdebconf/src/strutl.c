#include "common.h"
#include "strutl.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <wchar.h>

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
	char *start;

	for (; *c != 0 && isspace(*c); c++);

	if (*c == 0) 
		return 0;
	if (strlen(*inbuf) > maxlen)
		return 0;
	for (; *c != 0; c++)
	{
		if (*c == '"')
		{
			start = c+1;
			for (c++; *c != 0 && *c != '"'; c++)
			{
				if (*c == '\\')
				{
					c++;
					if (*c == 0)
						return 0;
				}
			}
			if (*c == 0)
				return 0;
			/* dequote the string */
			strunescape(start, buf, (int) (c - start + 1), 1);
			buf += strlen(buf);
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
	char *start = *inbuf;
	char *c;

	/* skip ws, return if empty */
	for (; *start != 0 && isspace(*start); start++); 
	if (*start == 0)
		return 0;

	c = start;
	
	for (; *c != 0 && isspace(*c) == 0; c++)
	{
		if (*c == '"')
		{
			for (c++; *c != 0 && *c != '"'; c++)
			{
				if (*c == '\\')
				{
					c++;
					if (*c == 0)
						return 0;
				}
			}
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
	strunescape(start, outbuf, (int) (c - start + 1), 1);

	/* skip trailing spaces */
	for (; *c != 0 && isspace(*c); c++);
	*inbuf = c;

	return 1;
}

int strchoicesplit(const char *inbuf, char **argv, size_t maxnarg)
{
    int argc = 0, i;
    const char *s = inbuf, *e, *c;
    char *p;

    if (inbuf == 0) return 0;

    INFO(INFO_VERBOSE, "Splitting [%s]\n", inbuf);
    while (*s != 0 && argc < maxnarg)
    {
        /* skip initial spaces */
        while (isspace(*s)) s++;

        /* find end */
        e = s;
        while (*e != 0)
        {
            if (*e == '\\' && *(e+1) == ',')
                e += 2;
            else if (*e == ',')
                break;
            else
                e++;
        }

        argv[argc] = malloc(e-s+1);
        for (c = s, i = 0; c < e; c++, i++)
        {
            if (*c == '\\' && c < (e-1) && *(c+1) == ',')
            {
                argv[argc][i] = ',';
                c++;
            }
            else
                argv[argc][i] = *c;
        }
        argv[argc][i] = 0;
        p = &argv[argc][i-1];
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

void strunescape(const char *inbuf, char *outbuf, const size_t maxlen, const int quote)
{
	const char *p = inbuf;
	int i = 0;
	while (*p != 0 && i < maxlen-1)
	{
		/*  Debconf only escapes \n  */
		if (*p == '\\')
		{
			if (*(p+1) == 'n')
			{
				outbuf[i++] = '\n';
				p += 2;
			}
			else if (quote != 0 && (*(p+1) == '"' || *(p+1) == '\\'))
			{
				outbuf[i++] = *(p+1);
				p += 2;
			}
			else
				outbuf[i++] = *p++;
		}
		else
			outbuf[i++] = *p++;
	}
	outbuf[i] = 0;
}

void strescape(const char *inbuf, char *outbuf, const size_t maxlen, const int quote)
{
	const char *p = inbuf;
	int i = 0;
	while (*p != 0 && i < maxlen-1)
	{
		/*  Debconf only escapes \n  */
		if (*p == '\n')
		{
			if (i + 2 >= maxlen) break;
			outbuf[i++] = '\\';
			outbuf[i++] = 'n';
			p++;
		}
		else if (quote != 0 && (*p == '"' || *p == '\\'))
		{
			if (i + 2 >= maxlen) break;
			outbuf[i++] = '\\';
			outbuf[i++] = *p++;
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

size_t
strwidth (const char *what)
{
    size_t res;
    int k;
    const char *p;
    wchar_t c;

    for (res = 0, p = what ; (k = mbtowc (&c, p, MB_LEN_MAX)) > 0 ; p += k)
        res += wcwidth (c);

    return res;
}  

