#ifndef _STRUTL_H_
#define _STRUTL_H_

#include <string.h>

int strcountcmp(const char *s1, const char *e1, const char *s2, const char *e2);
char *strtabexpand(char *buf, size_t buflen);
char *strstrip(char *buf);
void strvacat(char *buf, size_t len, ...);
int strparsecword(char **inbuf, char *outbuf, size_t maxlen);
int strparsequoteword(char **inbuf, char *outbuf, size_t maxlen);
int strcmdsplit(char *inbuf, char **argv, size_t maxnarg);
void strescape(const char *inbuf, char *outbuf, const size_t maxlen);

#endif
