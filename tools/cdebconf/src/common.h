#ifndef _COMMON_H_
#define _COMMON_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define DC_NOTOK	0
#define DC_OK		1
#define DC_NOTIMPL	2

#define DC_NO		0
#define DC_YES		1

#define INFO_ERROR	1
#define INFO_WARN	2
#define INFO_DEBUG	3

#define DIE(fmt, args...) 					\
 	do {							\
		fprintf(stderr, fmt, ##args);			\
		fprintf(stderr, " at %s line %u\n", __FILE__, __LINE__); \
		exit(1);					\
	} while(0)
#define INFO(level, fmt, args...)					\
 	do {							\
		fprintf(stderr, fmt, ##args);			\
		fprintf(stderr, " at %s line %u\n", __FILE__, __LINE__); \
	} while(0)

#define NEW(type) (type *)malloc(sizeof(type))
#define DELETE(x) do { free(x); x = 0; } while (0)
#define CHOMP(s) if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = 0 
#define STRDUP(s) ((s) == NULL ? NULL : strdup(s))

#endif
