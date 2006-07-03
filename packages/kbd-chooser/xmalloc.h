/* Error-free versions of some libc routines */


#include <stdlib.h>

#if defined(NDEBUG)

#define xmalloc(sz)      malloc(sz)
#define xrealloc(p,sz)   realloc(p,sz)
#define xstrdup(p)       strdup(p)
#define lkfatal(S)      do {} while(0)
#define lkfatal1(S1,S2) do {} while(0)
#define lkfatal0(S1,X)  do {} while(0)

#else

extern void *xmalloc(size_t sz);
extern void *xrealloc(void *p, size_t sz);
extern char *xstrdup(const char *p);
extern void lkfatal(const char *s);
extern void lkfatal1(const char *s, const char *s2);
extern void lkfatal0(const char *, int);

#endif
