
#ifndef CDEBCONF_DB_STACK
#define CDEBCONF_DB_STACK

struct question_stack {
  struct question_db *db;
  struct question_stack *next;
};

struct template_stack {
  struct template_db *db;
  struct template_stack *next;
};

#endif
