#ifndef _RFC822DB_H_
#define _RFC822DB_H_

struct template;
struct question;

struct template_db_cache {
	struct template *templates;
};

struct question_db_cache {
	struct question *questions;
};

struct rfc822_header {
    char *header;
    char *value;
    struct rfc822_header *next;
};

#endif
