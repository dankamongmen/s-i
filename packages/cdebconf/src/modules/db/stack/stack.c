/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: database.c
 *
 * Description: database interface routines
 *
 * $Id$
 *
 * cdebconf is (c) 2000-2001 Randolph Chung and others under the following
 * license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ***********************************************************************/
#include "common.h"
#include "database.h"
#include "configuration.h"
#include "question.h"
#include "template.h"
#include "priority.h"
#include "stack.h"

#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

/****************************************************************************
 *
 * Template database 
 *
 ***************************************************************************/

static int stack_template_db_initialize(struct template_db *db, struct configuration *cfg)
{
     char buf[256];
     struct template_stack *tstack = NULL;
     struct configitem *tmp, *child;
     
     tmp = cfg->tree(cfg, "template::instance::templatedb::stack");
     for (child = tmp->child; child != NULL; child = child->next) {
          if (tstack != NULL) {
               tstack->next = malloc(sizeof(struct template_stack));
               tstack = tstack->next;
          } else {
               tstack = malloc(sizeof(struct template_stack));
               db->data = tstack;
          }
          tstack->db = template_db_new(cfg, child->value);
     }

     return DC_OK;
}
     
static int stack_template_db_shutdown(struct template_db *db)
{
    struct template_stack *tstack = (struct template_stack *)db->data;
    while (tstack) {
         struct template_stack *next = tstack->next;          
         if (tstack->db->methods.shutdown(tstack->db) != DC_OK)
              return DC_NOTOK;
          dlclose(tstack->db->handle);
          DELETE(tstack->db);
        tstack = next;
    }
    return DC_OK;
}

static int stack_template_db_load(struct template_db *db) {
     int ret = DC_NOTOK;
     struct template_stack *tstack = (struct template_stack *)db->data;
     while (tstack) {
          /* ignore erors */
          if ((tstack->db->methods.load(tstack->db)) == DC_OK)
               ret = DC_OK;
          tstack = tstack->next;
     }
     return ret;
}

static int stack_template_db_save(struct template_db *db) {
    struct template_stack *tstack = (struct template_stack *)db->data;
    int ret = DC_NOTOK;
    while (tstack) {
         /* ignore erors */
         
         if (tstack->db->methods.save(tstack->db) == DC_OK)
              ret = DC_OK;
         tstack = tstack->next;
    }
    return ret;
}

/* TODO: compare with what we get from db_get and see if nothing has
 * changed. if so, just return.*/ 

static int stack_template_db_set(struct template_db *db, struct template *t)
{
     struct template_stack *tstack = (struct template_stack *)db->data;
     int ret;
     while (tstack) {
          ret = tstack->db->methods.set(tstack->db, t);
          switch (ret) {
          case DC_OK:
               return DC_OK;
          case DC_NOTOK:
               return DC_NOTOK;
          case DC_REJECT:
               tstack = tstack->next;
               break;
          }
     }
     /* everybody rejected it, it seems */
    return DC_REJECT;
}

static struct template *stack_template_db_get(struct template_db *db, 
	const char *name)
{
     struct template_stack *tstack = (struct template_stack *)db->data;
     struct template *t;
     while (tstack) {
          if ((t = tstack->db->methods.get(tstack->db, name)) != NULL) 
               return t;
          tstack = tstack->next;
     }
     return NULL;
}

static int stack_template_db_remove(struct template_db *db, const char *name)
{
     struct template_stack *tstack = (struct template_stack *)db->data;
     struct template *t;
     while (tstack) {
          t = tstack->db->methods.get(tstack->db, name);
          if (t) {
               template_deref(t);
               return tstack->db->methods.remove(tstack->db, name);
          }
          tstack = tstack->next;
     }
     /* nobody had it?  fail. */
    return DC_NOTOK;
}

static int stack_template_db_lock(struct template_db *db, const char *name)
{
	return DC_NOTIMPL;
}

static int stack_template_db_unlock(struct template_db *db, const char *name)
{
	return DC_NOTIMPL;
}

static struct template *stack_template_db_iterate(struct template_db *db, 
	void **iter)
{
	return 0;
}

/****************************************************************************
 *
 * Config database 
 *
 ***************************************************************************/

static int stack_question_db_initialize(struct question_db *db, struct configuration *cfg)
{
     struct question_stack *qstack = NULL;
     struct configitem *tmp, *child;
     char buf[256];

     tmp = cfg->tree(cfg, "config::instance::configdb::stack");
     for (child = tmp->child; child != NULL; child = child->next) {
          if (qstack != NULL) {
               qstack->next = malloc(sizeof(struct question_stack));
               qstack = qstack->next;
          } else {
               qstack = malloc(sizeof(struct question_stack));
               db->data = qstack;
          }
          qstack->db = question_db_new(cfg, db->tdb, child->value);
     }
     return DC_OK;
}

static int stack_question_db_shutdown(struct question_db *db)
{
    struct question_stack *qstack = (struct question_stack *)db->data;
    while (qstack) {
         struct question_stack *next = qstack->next;          
         if (qstack->db->methods.shutdown(qstack->db) != DC_OK)
              return DC_NOTOK;
          dlclose(qstack->db->handle);
          DELETE(qstack->db);
          qstack = next;
    }
    return DC_OK;
}

static int stack_question_db_load(struct question_db *db)
{
     struct question_stack *qstack = (struct question_stack *)db->data;
     while (qstack) {
          if (qstack->db->methods.load(qstack->db) != DC_OK) 
               return DC_NOTOK;
          qstack = qstack->next;
     }
     return DC_OK;
}

static int stack_question_db_save(struct question_db *db)
{
     struct question_stack *qstack = (struct question_stack *)db->data;
    while (qstack) {
         if (qstack->db->methods.save(qstack->db) == DC_NOTOK)
              return DC_NOTOK;
         qstack = qstack->next;
    }
    return DC_OK;
}

/* TODO: compare with what we get from db_get and see if nothing has
 * changed. if so, just return.*/ 

static int stack_question_db_set(struct question_db *db, struct question *t) {
     struct question_stack *qstack = (struct question_stack *)db->data;
     int ret;
     while (qstack) {
          ret = qstack->db->methods.set(qstack->db, t);
          switch (ret) {
          case DC_OK:
               return DC_OK;
          case DC_NOTOK:
               return DC_NOTOK;
          case DC_REJECT:
               qstack = qstack->next;
               break;
          }
     }
     /* everybody rejected it, it seems */
    return DC_REJECT;
}

static struct question *stack_question_db_get(struct question_db *db, 
	const char *name)
{
     struct question_stack *qstack = (struct question_stack *)db->data;
     struct question *q;
     while (qstack) {
          if ((q = qstack->db->methods.get(qstack->db, name)) != NULL) 
               return q;
          qstack = qstack->next;
     }
     return NULL;
}

static int stack_question_db_disown(struct question_db *db, const char *name, 
	const char *owner)
{
     struct question_stack *qstack = (struct question_stack *)db->data;
     struct question *q;
     int ret;
     while (qstack) {
          /* only disown those which actually exist in the db */
          if ((q = qstack->db->methods.get(qstack->db, name)) != NULL) {
               question_deref(q);
               ret = qstack->db->methods.disown(qstack->db, name, owner);
               if (ret != DC_OK)
                    return ret;
          }
          qstack = qstack->next;
     }
     return DC_OK;
}

static int stack_question_db_disownall(struct question_db *db, const char *owner)
{
     struct question_stack *qstack = (struct question_stack *)db->data;
     int ret;
     while (qstack) {
          ret = qstack->db->methods.disownall(qstack->db, owner);
          if (ret != DC_OK)
               return ret;
          qstack = qstack->next;
     }
     return DC_OK;
}

static int stack_question_db_lock(struct question_db *db, const char *name)
{
	return DC_NOTIMPL;
}

static int stack_question_db_unlock(struct question_db *db, const char *name)
{
	return DC_NOTIMPL;
}

static struct question *stack_question_db_iterate(struct question_db *db,
	void **iter)
{
	return 0;
}

struct template_db_module debconf_template_db_module = {
    initialize: stack_template_db_initialize,
    shutdown: stack_template_db_shutdown,
    load: stack_template_db_load,
    save: stack_template_db_save,
    set: stack_template_db_set,
    get: stack_template_db_get,
    remove: stack_template_db_remove,
    iterate: stack_template_db_iterate,
    lock: stack_template_db_lock,
    unlock: stack_template_db_unlock,
};

struct question_db_module debconf_question_db_module = {
    initialize: stack_question_db_initialize,
    shutdown: stack_question_db_shutdown,
    load: stack_question_db_load,
    save: stack_question_db_save,
    set: stack_question_db_set,
    get: stack_question_db_get,
    disown: stack_question_db_disown,
    disownall: stack_question_db_disownall,
    iterate: stack_question_db_iterate,
    lock: stack_question_db_lock,
    unlock: stack_question_db_unlock,
};
