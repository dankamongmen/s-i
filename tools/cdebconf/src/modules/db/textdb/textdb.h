#ifndef _TEXTDB_H_
#define _TEXTDB_H_

#define TEXTDB_TEMPLATE_PATH	"./templates"
#define TEXTDB_QUESTION_PATH	"./questions"

struct template;
struct question;

struct db_cache {
	struct template *templates;
	struct question *questions;
};

#endif
