#ifndef _STRUTL_H_
#define _STRUTL_H_

#include <string.h>

int strcountcmp(const char *s1, const char *e1, const char *s2, const char *e2);
char *strtabexpand(char *buf, size_t buflen);
char *strstrip(char *buf);
char *strlower(char *buf);
void strvacat(char *buf, size_t len, ...);
int strparsecword(char **inbuf, char *outbuf, size_t maxlen);
int strparsequoteword(char **inbuf, char *outbuf, size_t maxlen);
int strchoicesplit(const char *inbuf, char **argv, size_t maxnarg);
int strcmdsplit(char *inbuf, char **argv, size_t maxnarg);
void strunescape(const char *inbuf, char *outbuf, const size_t maxlen);
void strescape(const char *inbuf, char *outbuf, const size_t maxlen);
int strwrap(const char *str, const int width, char *lines[], int maxlines);
int strlongest(char **strs, int count);

#endif
