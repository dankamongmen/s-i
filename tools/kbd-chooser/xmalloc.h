/* Error-free versions of some libc routines */


#if defined(NDEBUG)

#define xmalloc(sz)      malloc(sz)
#define xrealloc(p,sz)   realloc(p,sz)
#define xstrdup(p)       strdup(p)

#else

extern void *xmalloc(size_t sz);
extern void *xrealloc(void *p, size_t sz);
extern char *xstrdup(char *p);

#endif
