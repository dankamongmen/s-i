#include "common.h"
#include "configuration.h"
#include "database.h"
#include "rfc822db.h"
#include "template.h"
#include "question.h"
#include "strutl.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

static char *unescapestr(const char *in)
{
	static char buf[8192];
	if (in == 0) return 0;
	strunescape(in, buf, sizeof(buf));
	return buf;
}

static char *escapestr(const char *in)
{
	static char buf[8192];
	if (in == 0) return 0;
	strescape(in, buf, sizeof(buf));
	return buf;
}

static void parse_variables(struct question *q, char *string)
{
        char *wc, *owc;
        if (!string)
                return;
        owc = wc = strdup(string);
        while (wc != NULL && *wc != '\0')
        {
                char *delim = wc;
                char *striptmp_var, *striptmp_val;
                int finished = 0;
                while (*delim != '=' && *delim != '\0')
                        delim++;
                if (*delim == '\0')
                        finished = 1;
                *delim = '\0';

                striptmp_var = strdup(wc);

                delim++;
                wc = delim;
                while (*delim != '\n' && *delim != '\0')
                        delim++;
                if (*delim == '\0')
                        finished = 1;
                *delim = '\0';

                striptmp_val = strdup(wc);
                question_variable_add(q,striptmp_var, striptmp_val);

                free(striptmp_val);
                free(striptmp_var);

                wc = delim;
                wc++;
                if (finished)
                        break;

                while (*wc == ' ' || *wc == '\t')
                {
                        wc++;
                }
                
        }
        free(owc);
}

static void parse_owners(struct question *q, char *string)
{
        char *wc = strdup(string);
        while (wc != NULL)
        {
                char *delim = wc;
                int finished = 0;
                while (*delim != ',' && *delim != '\0')
                        delim++;
                if (*delim == '\0')
                        finished = 1;
                *delim = '\0';
                question_owner_add(q,wc);
                if (finished)
                        break;
                wc = delim;
                while (*wc == ' ' || *wc == '\t' || *wc == '\0')
                {
                        wc++;
                }
                
        }
        free(wc);
}

static int parse_flags(char *string)
{
        int ret = 0;
        if (string == NULL)
                return ret;
        if (strstr(string, "seen") != NULL)
        {
                ret |= DC_QFLAG_SEEN;
        }
        return ret;
}

static int rfc822db_initialize(struct database *db, struct configuration *cfg)
{
	struct db_cache *dbdata;
        /*        fprintf(stderr,"rfc822db_initialize(db,cfg)\n");*/
	dbdata = malloc(sizeof(struct db_cache));

	if (dbdata == NULL)
		return DC_NOTOK;

	dbdata->questions = NULL;
	dbdata->templates = NULL;
	db->data = dbdata;

	return DC_OK;
}

/*
 * Function: rc822db_parse_stanza
 * Input: a FILE pointer to an open readable file containing a stanza in rfc822 
 *        format.
 * Output: a pointer to a dynamically allocated rfc822_header structure
 * Description: parse a stanza from file into the returned header struct
 * Assumptions: no lines are over 8192 bytes long.
 */

static struct rfc822_header* rfc822db_parse_stanza(FILE *file)
{
        struct rfc822_header *head;
        struct rfc822_header *ret = NULL;
        char buf[8192+1];
        /*        fprintf(stderr,"rfc822db_parse_stanza(file)\n");*/
        while (!feof(file)) 
        {
                /* don't free tmp, it will point inside buf */
                char *tmp;
                
                tmp = fgets(buf, 8192, file);
                if (tmp == NULL || *tmp == '\n')
                        break;
                if (*tmp == ' ' || *tmp == '\t')
                {
                        /* continuation line, just append it */
                        int len = strlen(tmp) + strlen(head->value) + 1;
                        char *tmp2 = malloc(len);
                        sprintf(tmp2,"%s%s", head->value, tmp);
                        free(head->value);
                        head->value = tmp2;
                        continue;
                } else {
                        while (*tmp != ':')
                                tmp++;
                        *tmp = '\0';
                        tmp++;
                        head = malloc(sizeof(struct rfc822_header));
                        if (!head)
                                return NULL;
                        memset(head, '\0',sizeof(struct rfc822_header));        
                        head->header = strdup(buf);
                        while(*tmp == ' ' || *tmp == '\t')
                                tmp++;
                        head->value = strstrip(unescapestr(tmp));
                        head->next = ret;
                        ret = head;
                }
                if (head->value[strlen(head->value)-1] == '\n') {
                  /* one byte memory leak.  FIXME */
                  head->value[strlen(head->value)-1] = '\0';
                }
        }
        return ret;
}

static char *rfc822db_header_lookup(struct rfc822_header *list, char* key)
{
/*        fprintf(stderr,"rfc822db_header_lookup(list,key=%s)\n",key);*/
        while (list && (strcasecmp(key,list->header) != 0))
                list = list->next;
        if (!list)
                return NULL;
/*        fprintf(stderr,"rfc822db_header_lookup returning: '%s'\n", list->value);*/
        return list->value;
}

/*
 * Function: rfc822db_parse_templates
 * Input: a template struct, an open readable FILE containing the templates to 
 *        be parsed
 * Output: a template struct.  Undefined if no rfc822 stanzas could be found
 * Description: parse a complete file in rfc822 format into a template struct.
 * Assumptions: the file is in valid rfc822 format
 * Todo: handle localized templates
 */

static struct template* rfc822db_parse_templates(struct template *templates, 
                                                 FILE* file)
{
        struct rfc822_header *header = NULL;
        /*        fprintf(stderr,"rfc822db_parse_templates(templates,file)\n");*/
        while ((header = rfc822db_parse_stanza(file)) != NULL)
        {
                struct template *tmp;
                if (rfc822db_header_lookup(header,"name") == NULL)
                        continue;
                tmp = template_new(rfc822db_header_lookup(header,"name"));

                tmp->type = rfc822db_header_lookup(header,"type");
                tmp->defaultval = rfc822db_header_lookup(header,"default");
                tmp->choices = rfc822db_header_lookup(header,"choices");
                tmp->description = rfc822db_header_lookup(header,"description");
                tmp->extended_description = rfc822db_header_lookup(header,"extended_description");
/*	struct language_description *localized_descriptions; */
                tmp->next = templates;
                templates = tmp;

        }
        return templates;
}

/*
 * Function: rfc822db_parse_questions
 * Input: a question struct, an open readable FILE containing the questions to 
 *        be parsed
 * Output: a question struct.  Undefined if no rfc822 stanzas could be found
 * Description: parse a complete file in rfc822 format into a question struct.
 * Assumptions: the file is in valid rfc822 format
 */

static struct question* rfc822db_parse_questions(struct question *questions,
                                                 FILE* file)
{
        struct rfc822_header *header = NULL;
        /*        fprintf(stderr,"rfc822db_parse_questions(questions, file)\n");*/
        while ((header = rfc822db_parse_stanza(file)) != NULL)
        {
                struct question *tmp;
                tmp = question_new(rfc822db_header_lookup(header,"name"));
                question_setvalue(tmp, rfc822db_header_lookup(header,"value"));
/*                tmp->defaultval = rfc822db_header_lookup(header,"variables");*/
                tmp->flags = parse_flags(rfc822db_header_lookup(header,"flags"));
                parse_owners(tmp,rfc822db_header_lookup(header,"owners"));
                parse_variables(tmp,rfc822db_header_lookup(header,"variables"));
                tmp->next = questions;
                if (tmp->next != NULL)
                        tmp->next->prev = tmp;
                questions = tmp;
        }
        return questions;
}

static struct template *find_template(struct template *t, const char *name)
{
        while (t != NULL)
        {
                if (strcmp(name,t->tag) == 0)
                        return t;
                t = t->next;
        }
        return NULL;
}

static struct question *find_question(struct question *q, const char *name)
{
        while (q != NULL)
        {
                if (strcmp(name,q->tag) == 0)
                        return q;
                q = q->next;
        }
        return NULL;
}

static void rfc822db_connect_questions_templates(struct db_cache *dbdata)
{
        struct template *t = dbdata->templates;
        struct question *q = dbdata->questions;
        while (q != NULL)
        {
                q->template = find_template(t,q->tag);
                q = q->next;
        }
        while (t != NULL)
        {
                if (find_question(dbdata->questions, t->tag) == NULL)
                {
                        q = question_new(t->tag);
                        q->template = t;
                        q->next = dbdata->questions;
                        dbdata->questions = q;
                }
                t = t->next;
        }
}

static int rfc822db_load(struct database *db)
{
        struct db_cache *dbdata = db->data;
        FILE *inf;

        /*        fprintf(stderr,"rfc822db_load(db)\n");*/
        if ((inf = fopen(db->config->get(
                                 db->config, 
                                 "database::driver::rfc822db::templatepath", 
                                 RFC822DB_TEMPLATE_PATH), "r")) == NULL)
                return DC_NOTOK;
        dbdata->templates = rfc822db_parse_templates(dbdata->templates,inf);
        fclose(inf);

        if ((inf = fopen(db->config->get(
                                 db->config, 
                                 "database::driver::rfc822db::questionpath", 
                                 RFC822DB_QUESTION_PATH),"r")) == NULL)
                return DC_NOTOK;
        dbdata->questions = rfc822db_parse_questions(dbdata->questions,inf);
        rfc822db_connect_questions_templates(dbdata);
        fclose(inf);

	return DC_OK;
}

static int rfc822db_dump_templates(struct template *t, FILE *outf)
{
  /*        fprintf(stderr,"rfc822db_dump_templates(t=%p, outf=%p)\n",t,outf);*/
        while (t)
        {
                struct language_description *langdesc;
                /*                fprintf(stderr,"dumping template %s\n",t->tag);*/
                fprintf(outf, "Name: %s\n", escapestr(t->tag));
                fprintf(outf, "Type: %s\n", escapestr(t->type));
/*	if (t->defaultval != NULL)
        fprintf(outf, "\tdefault \"%s\";\n", escapestr(t->defaultval));
*/
                if (t->choices != NULL)
                        fprintf(outf, "Choices: %s\n", escapestr(t->choices));
                if (t->description != NULL)
                        fprintf(outf, "Description: %s\n", escapestr(t->description));
                if (t->extended_description != NULL)
                        fprintf(outf, "Extended_description: %s\n", escapestr(t->extended_description));
                
                langdesc = t->localized_descriptions;
                while (langdesc) 
                {
                        if (langdesc->description != NULL) 
                                fprintf(outf, "Description-%s: %s\n", 
                                        langdesc->language, 
                                        escapestr(langdesc->description));
                        
                        if (langdesc->extended_description != NULL)
                                fprintf(outf, "Extended_description-%s: %s\n", 
                                        langdesc->language, 
                                        escapestr(langdesc->extended_description));
                        
                        if (langdesc->choices != NULL) 
                                fprintf(outf, "Choices-%s: %s\n", 
                                        langdesc->language, 
                                        escapestr(langdesc->choices));
                        
                        langdesc = langdesc->next;
                }
                fprintf(outf, "\n");
                t = t->next;
        }
        return DC_OK;
}

static int rfc822db_dump_questions(struct question *q, FILE *outf)
{
	struct questionvariable *var;
	struct questionowner *owner;
        /*        fprintf(stderr,"rfc822db_dump_questions(q, outf)\n");*/
        while (q)
        {
          /*                fprintf(stderr, "Dumping question: %s\n", q->tag);*/
                fprintf(outf, "Name: %s\n", escapestr(q->tag));
                fprintf(outf, "Value: %s\n", (q->value ? escapestr(q->value) : ""));
/*	if (q->defaultval)
        fprintf(outf, "\tdefault \"%s\";\n", escapestr(q->defaultval));
*/
//	fprintf(outf, "\tflags 0x%08X;\n", q->flags);
                
                fprintf(outf, "Template: %s\n", escapestr(q->tag));
                if ((var = q->variables))
                {
                        fprintf(outf, "Variables:\n");
                        while (var)
                        {
                                /* escapestr uses a static buf, so we do this in
                                 * two steps */
                                fprintf(outf, " %s = ", (var->variable ? escapestr(var->variable) : ""));
                                fprintf(outf, "%s\n", (var->value ? escapestr(var->value) : ""));

                                var = var->next;
                        }
                }
                if ((owner = q->owners))
                {
                        fprintf(outf, "Owners: ");
                        do {
                                fprintf(outf, "%s", escapestr(owner->owner));
                        } while ((owner = owner->next));
                        fprintf(outf, "\n");
                }
                fprintf(outf, "\n");
                /*                fprintf(stderr, "Dumped: %s, next: %p\n", q->tag, q->next);*/
                q = q->next;

        }
        return DC_OK;
}

static int rfc822db_save(struct database *db)
{
        struct db_cache *dbdata = db->data;
        FILE *outf = NULL;
        /*        fprintf(stderr,"rfc822save(db)\nwriting to %s\n", 
                db->config->get(
                        db->config, 
                        "database::driver::rfc822db::templatepath", 
                        RFC822DB_TEMPLATE_PATH));*/
        if ((outf = fopen(db->config->get(
                                 db->config, 
                                 "database::driver::rfc822db::templatepath", 
                                 RFC822DB_TEMPLATE_PATH), "w")) == NULL)
                return DC_NOTOK;
        rfc822db_dump_templates(dbdata->templates,outf);
        if (fclose(outf) == EOF)
                perror("closing after dump_templates");

        if ((outf = fopen(db->config->get(
                                  db->config, 
                                  "database::driver::rfc822db::questionpath", 
                                  RFC822DB_QUESTION_PATH), "w")) == NULL)
                return DC_NOTOK;
        rfc822db_dump_questions(dbdata->questions,outf);
        if (fclose(outf) == EOF)
                perror("closing after dump_questions");
	return DC_OK;
}

static int rfc822db_template_set(struct database *db, struct template *template)
{
        struct template *t = template_dup(template);
        struct db_cache *dbdata = db->data;
        struct template *db_prev = dbdata->templates;
        /* the template in the db, if already there */
        struct template *db_t = dbdata->templates;

        fprintf(stderr,"rfc822db_template_set(db,t=%s)\n", t->tag);
        while (db_t)
        {
                if (strcmp(t->tag,db_t->tag) == 0) 
                {
                        /* match, replace */
                        /* leak a template FIXME */
                        
                        db_prev->next = t;
                        t->next = db_t->next;
                        return DC_OK;
                }
                db_prev = db_t;
                db_t = db_t->next;
        }

        /* not in the list yet, so..  insert t at the beginning */

/*                fprintf(stderr,"\t%s not found, inserting at head\n",t->tag);*/
        t->next = dbdata->templates;
        dbdata->templates = t;
	return DC_OK;
}

static struct template *rfc822db_template_get(struct database *db, 
	const char *ltag)
{
        struct db_cache *dbdata = db->data;
        struct template *t = dbdata->templates;
        /*        fprintf(stderr,"rfc822db_template_get(db,ltag=%s)\n",ltag);*/
        return find_template(t, ltag);
}

static int rfc822db_template_remove(struct database *db, const char *tag)
{
        struct db_cache *dbdata = db->data;
        struct template *t = dbdata->templates;
        struct template *prev = NULL;
        /*        fprintf(stderr,"rfc822db_template_remove(db,tag=%s)\n",tag);*/
        while (t)
        {
                if (strcmp(t->tag, tag) == 0)
                {
                        /* free the template as well, FIXME */
                        if (prev)
                                prev->next = t->next;
                        else 
                                dbdata->templates = t->next;
                        return DC_OK;
                }
                prev = t;
                t = t->next;
        }
        
        return DC_NOTOK;
}

static struct template *rfc822db_template_iterate(struct database *db,
	void **iter)
{
        fprintf(stderr,"rfc822db_template_iterate stub\n");
        return NULL;
}

static int rfc822db_question_set(struct database *db, struct question *question)
{
        struct question *q = question_dup(question);
        struct db_cache *dbdata = db->data;
        struct question *db_prev = dbdata->questions;

        /* the question in the db, if already there */
        struct question *db_q = dbdata->questions;

        fprintf(stderr,"rfc822db_question_set(db,t=%s)\n", q->tag);

        q->template = find_template(dbdata->templates, q->tag);
        while (db_q)
        {
                if (strcmp(q->tag,db_q->tag) == 0) 
                {
                        /* match, replace */
                        q->next = db_q->next;
                        /* leak a question FIXME */
                        db_prev->next = q;
                        return DC_OK;
                }
                db_prev = db_q;
                db_q = db_q->next;
        }
        /* not in the list yet, so insert q at the beginning */
        /*        fprintf(stderr,"\t%s not found, inserting at head\n",q->tag);*/
        q->next = dbdata->questions;
        dbdata->questions = q;
        return DC_OK;
}



static struct question *rfc822db_question_get(struct database *db, 
	const char *ltag)
{
        struct db_cache *dbdata = db->data;
        struct question *q = dbdata->questions;
        /*        fprintf(stderr,"rfc822db_question_get(db,ltag=%s)\n",ltag);*/
        while (q)
        {
                if (strcmp(q->tag,ltag) == 0)
                {
                        if (q->tag == 0 || q->value == 0 || q->template == 0)
                                return NULL;

                        return question_dup(q);
                }
                q = q->next;
        }
        /*        fprintf(stderr,"rfc822db_question_get: couldn't find: %s\n", ltag);*/
        return NULL;
}

static int rfc822db_question_disown(struct database *db, const char *tag, 
	const char *owner)
{
	struct question *q = rfc822db_question_get(db, tag);
        /*        fprintf(stderr,"rfc822db_question_disown(db, tag=%s, owner=%s)\n",
                  tag,owner);*/
	if (q == NULL) return DC_NOTOK;
	question_owner_delete(q, owner);
	rfc822db_question_set(db, q);
	question_deref(q);
	return DC_OK;
}

static struct question *rfc822db_question_iterate(struct database *db,
	void **iter)
{
          fprintf(stderr,"rfc822db_question_iterate stub\n");
        return NULL;
}

int main(int argc, char** argv) {
  FILE *in;
  struct question *t = NULL;
  /*  struct rfc822_header *head; */
  in = fopen("testdata", "r");
  t = rfc822db_parse_questions(t,in);

  while (t) {
          struct questionowner *q = t->owners;
          struct questionvariable *v = t->variables;
          printf("\nname: %s\nvalue: %s\n", t->tag, t->value);
          while(q) {
                  printf("owner: %s\n",q->owner);
                  q = q->next;
          }

          while(v) {
                  printf("vars: %s = %s\n",v->variable, v->value);
                  v = v->next;
          }

    t = t->next;
  }

  /*  head = rfc822db_parse_stanza(in);
  while (head != NULL) {
    printf("key: %s\nvalue:%s\n", head->header, head->value);
    head = head->next;
    }*/
  return 0;
}


struct database_module debconf_database_module =
{
	initialize: rfc822db_initialize,
	load: rfc822db_load,
	save: rfc822db_save,

	template_set: rfc822db_template_set,
	template_get: rfc822db_template_get,
	template_remove: rfc822db_template_remove,
	template_iterate: rfc822db_template_iterate,

	question_get: rfc822db_question_get,
	question_set: rfc822db_question_set,
	question_disown: rfc822db_question_disown,
	question_iterate: rfc822db_question_iterate
};
