#ifndef _RFC822DB_H_
#define _RFC822DB_H_

#define RFC822DB_TEMPLATE_PATH	"./templates.dat"
#define RFC822DB_QUESTION_PATH	"./config.dat"

struct template;
struct question;

struct db_cache {
	struct template *templates;
	struct question *questions;
};

struct rfc822_header {
  char *header;
  char *value;
  struct rfc822_header *next;
};

#endif
