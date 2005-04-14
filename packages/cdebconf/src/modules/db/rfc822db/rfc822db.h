#ifndef _RFC822DB_H_
#define _RFC822DB_H_

#include <debian-installer/slist.h>
#include "rfc822.h"

struct template;
struct question;

struct template_db_cache {
        /* struct table_t *hash;*/
        void *root;
        di_slist *iterator;
};

struct question_db_cache {
        /* struct table_t *hash; */
        void *root;
        di_slist *iterator;
};

#endif
