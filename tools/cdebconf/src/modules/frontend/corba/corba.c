#include "common.h"
#include "template.h"
#include "question.h"
#include "frontend.h"
#include "database.h"
#include "strutl.h"

#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "dcf.h"

#define UIDATA(f) ((struct uidata *)(f)->data)

#define EXCEPTION(ev) 								\
  if ((ev)->_major != CORBA_NO_EXCEPTION) { 					\
    if ((ev)->_major == CORBA_USER_EXCEPTION) {					\
      CORBA_char *buffer; 							\
      buffer = CORBA_exception_id(ev);	 					\
      if (strcmp (buffer, "Debconf_Frontend_Failure")) { 			\
	fprintf(stderr, "dcf_corba: user exception: %s\n", 			\
		(CORBA_exception_value(ev))->reason); 				\
	return DC_NOTOK; 							\
      } else { 									\
	fprintf(stderr, "dcf_corba: unknown exception raised!!\n");	 	\
	return DC_NOTOK; 							\
      } 									\
    } else { 									\
      fprintf(stderr, "dcf_corba: System exception: %s\n", 			\
	      CORBA_exception_id(ev)); 						\
      return DC_NOTOK; 								\
    } 										\
  }


/* Private variables */
struct uidata {
  CORBA_Environment *ev;
  CORBA_Object *serv;
};

static int corba_boolean(struct frontend *f, struct question *q) {
  struct uidata *uid = UIDATA(f);
  CORBA_Environment *ev = uid->ev;

  CORBA_boolean ans = Debconf_Frontend_Boolean(*uid->serv, "yes/no", ev);
  EXCEPTION(ev);

  return DC_OK;
}

static int corba_multiselect(struct frontend *f, struct question *q) {
  struct uidata *uid = UIDATA(f);
  CORBA_Environment *ev = uid->ev;

  CORBA_boolean ans = Debconf_Frontend_Multiselect(*uid->serv, "yes/no", ev);
  EXCEPTION(ev);

  return DC_OK;
}

static int corba_note(struct frontend *f, struct question *q) {
  struct uidata *uid = UIDATA(f);
  CORBA_Environment *ev = uid->ev;

  CORBA_boolean ans = Debconf_Frontend_Note(*uid->serv, "yes/no", ev);
  EXCEPTION(ev);

  return DC_OK;
}

static int corba_password(struct frontend *f, struct question *q) {
  struct uidata *uid = UIDATA(f);
  CORBA_Environment *ev = uid->ev;

  CORBA_boolean ans = Debconf_Frontend_Password(*uid->serv, "yes/no", ev);
  EXCEPTION(ev);

  return DC_OK;
}

static int corba_select(struct frontend *f, struct question *q) {
  struct uidata *uid = UIDATA(f);
  CORBA_Environment *ev = uid->ev;

  CORBA_boolean ans = Debconf_Frontend_Select(*uid->serv, "yes/no", ev);
  EXCEPTION(ev);

  return DC_OK;
}

static int corba_string(struct frontend *f, struct question *q) {
  struct uidata *uid = UIDATA(f);
  CORBA_Environment *ev = uid->ev;

  CORBA_boolean ans = Debconf_Frontend_String(*uid->serv, "yes/no", ev);
  EXCEPTION(ev);

  return DC_OK;
}

static int corba_text(struct frontend *f, struct question *q) {
  struct uidata *uid = UIDATA(f);
  CORBA_Environment *ev = uid->ev;

  CORBA_boolean ans = Debconf_Frontend_Text(*uid->serv, "yes/no", ev);
  EXCEPTION(ev);

  question_setvalue(q, (ans ? "true" : "false"));
  return DC_OK;
}

/******************************************************************************/

struct question_handlers {
  const char *type;
  int (*handler)(struct frontend *f, struct question *q);
} question_handlers[] = {
  { "boolean", 	corba_boolean },
  { "multiselect", corba_multiselect },
  { "note", 	corba_note },
  { "password", corba_password },
  { "select", 	corba_select },
  { "string", 	corba_string },
  { "text", 	corba_text }
};

int corba_initialize(struct frontend *f, struct configuration *conf)
{
  struct uidata *uid;
  CORBA_ORB orb;
  gchar *dummy_argv[2];
  gint dummy_argc;

  dummy_argc = 1;
  dummy_argv[0] = argv[0];
  dummy_argv[1] = 0;

  uid = (struct uidata *) malloc(sizeof(struct uidata));
  if (! uid)
    return DC_NOTOK;
  memset(uid, 0, sizeof(struct uidata));
  uid->ev = (CORBA_Environment *) malloc(sizeof(CORBA_Environment));
  if (! uid->ev)
    return DC_NOTOK;
  uid->serv = (CORBA_Object *) malloc(sizeof(CORBA_Object));
  if (! uid->serv)
    return DC_NOTOK;

  f->interactive = 1;

  CORBA_exception_init(uid->ev);
  orb = CORBA_ORB_init(&dummy_argc, dummy_argv, "orbit-local-orb", uid->ev);
  if (uid->ev->_major != CORBA_NO_EXCEPTION) {
    fprintf(stderr, "Error: unable to initialise the ORB: %s\n", 
	    CORBA_exception_id(uid->ev));
    CORBA_exception_free(uid->ev);
    free(uid->ev);
    free(uid->serv);
    free(uid);
    return DC_NOTOK;
  } 
  *uid->serv = CORBA_ORB_string_to_object(orb, argv[1], uid->ev);

  f->data = uid;
  
  return DC_OK;
}

static int corba_shutdown(struct frontend *f) {
  struct uidata *uid = UIDATA(f);
  CORBA_Object_release(&uid->serv, uid->ev);
  CORBA_exception_free(uid->ev);
  free(uid->ev);
  free(uid->serv);
  free(uid);

  return DC_OK;
}

static int corba_go(struct frontend *f) {
  struct uidata *uid = UIDATA(f);
  struct question *q = f->questions;
  int i;
  int ret;
  
  printf("%s\n\n", f->title);
  
  for (; q != 0; q = q->next) {
    for (i = 0; i < DIM(question_handlers); i++)
      if (strcmp(q->template->type, question_handlers[i].type) == 0) {
	corba_displaydesc(f, q);
	ret = question_handlers[i].handler(f, q);
	if (ret == DC_OK)
	  f->db->question_set(f->db, q);
	else
	  return ret;
	break;
      }
  }
  
  return DC_OK;
}

struct frontend_module debconf_frontend_module = {
  initialize: corba_initialize,
  shutdown: corba_shutdown,
  go: corba_go,
};
