#ifndef _RFC822DB_H_
#define _RFC822DB_H_

#include <table.h>
struct template;
struct question;

struct template_db_cache {
        struct table_t *hash;
};

struct question_db_cache {
        struct table_t *hash;
};

struct rfc822_header {
    char *header;
    char *value;
    struct rfc822_header *next;
};

#endif
