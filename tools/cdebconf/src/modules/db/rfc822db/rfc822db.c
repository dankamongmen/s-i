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
#include <ctype.h>
#include <time.h>
#include <errno.h>

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

        striptmp_var = strdup(strstrip(wc));

        delim++;
        wc = delim;
        while (*delim != '\n' && *delim != '\0')
            delim++;
        if (*delim == '\0')
            finished = 1;
        *delim = '\0';

        striptmp_val = strdup(strstrip(wc));
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
    char *wc, *owc;
    
    owc = wc = strdup(string);

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

    free(owc);
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

/*
 * Function: rc822db_parse_stanza
 * Input: a FILE pointer to an open readable file containing a stanza in rfc822 
 *    format.
 * Output: a pointer to a dynamically allocated rfc822_header structure
 * Description: parse a stanza from file into the returned header struct
 * Assumptions: no lines are over 8192 bytes long.
 */

static struct rfc822_header* rfc822db_parse_stanza(FILE *file)
{
    struct rfc822_header *head, **tail, *cur;
    char buf[8192];

    head = NULL;
    tail = &head;
    cur = NULL;

    /*    fprintf(stderr,"rfc822db_parse_stanza(file)\n");*/
    while (fgets(buf, sizeof(buf), file))
    {
        char *tmp = buf;

        if (*tmp == '\n')
            break;

        CHOMP(buf);

        if (isspace(*tmp))
        {
            /* continuation line, just append it */
            int len;

            if (cur == NULL)
                break; /* should report an error here */

            len = strlen(cur->value) + strlen(tmp) + 2;

            cur->value = realloc(cur->value, len);
            strvacat(cur->value, len, "\n", tmp, NULL);
        } 
        else 
        {
            while (*tmp != 0 && *tmp != ':')
                tmp++;
            *tmp++ = 0;

            cur = NEW(struct rfc822_header);
            if (cur == NULL)
                return NULL;
            memset(cur, '\0',sizeof(struct rfc822_header));    

            cur->header = strdup(buf);

            while (isspace(*tmp))
                tmp++;

            cur->value = strdup(unescapestr(tmp));

            *tail = cur;
            tail = &cur->next;
        }
    }

    return head;
}


static char *rfc822db_header_lookup(struct rfc822_header *list, const char* key)
{
/*    fprintf(stderr,"rfc822db_header_lookup(list,key=%s)\n",key);*/
    while (list && (strcasecmp(key, list->header) != 0))
        list = list->next;
    if (!list)
        return NULL;
/*    fprintf(stderr,"rfc822db_header_lookup returning: '%s'\n", list->value);*/
    return list->value;
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

/* templates */
static int rfc822db_template_initialize(struct template_db *db, struct configuration *cfg)
{
    struct template_db_cache *dbdata;
    /*    fprintf(stderr,"rfc822db_initialize(db,cfg)\n");*/
    dbdata = malloc(sizeof(struct template_db_cache));

    if (dbdata == NULL)
        return DC_NOTOK;

    dbdata->templates = NULL;
    db->data = dbdata;

    return DC_OK;
}

/*
 * Function: rfc822db_template_load
 * Input: template database
 * Output: DC_OK/DC_NOTOK
 * Description: parse a template db file and put it into the cache
 * Assumptions: the file is in valid rfc822 format
 * TODO: handle localized templates; don't use linked lists, it's
 *       slow
 */
static int rfc822db_template_load(struct template_db *db)
{
    struct template_db_cache *dbdata = db->data;
    char tmp[1024];
    const char *path;
    FILE *inf;
    struct rfc822_header *header = NULL;
    struct template **templates;

    templates = &dbdata->templates;
    while (*templates != NULL)
        templates = &(*templates)->next;

    snprintf(tmp, sizeof(tmp), "%s::path", db->configpath);
    path = db->config->get(db->config, tmp, 0);
    if (path == NULL ||
        (inf = fopen(path, "r")) == NULL)
    {
        INFO(INFO_ERROR, "Cannot open template file %s\n",
            path ? path : "<empty>");
        return DC_NOTOK;
    }

    while ((header = rfc822db_parse_stanza(inf)) != NULL)
    {
        struct template *tmp;
        const char *name;

        name = rfc822db_header_lookup(header, "name");

        if (name == NULL)
        {
            INFO(INFO_ERROR, "Read a stanza without a name\n");
            DELETE(header);
            continue;
        }

        tmp = template_new(name);

        tmp->type = rfc822db_header_lookup(header, "type");
        tmp->defaultval = rfc822db_header_lookup(header, "default");
        tmp->choices = rfc822db_header_lookup(header, "choices");
        tmp->description = rfc822db_header_lookup(header, "description");
        tmp->extended_description = rfc822db_header_lookup(header, "extended_description");
/*  struct language_description *localized_descriptions; */
        tmp->next = NULL;

        *templates = tmp;
        templates = &tmp->next;

        DELETE(header);
    }

    fclose(inf);

    return DC_OK;
}

static int rfc822db_template_save(struct template_db *db)
{
    FILE *outf;
    struct template_db_cache *dbdata = db->data;
    char tmp[1024];
    const char *path;
    struct template *t;

    snprintf(tmp, sizeof(tmp), "%s::path", db->configpath);
    path = db->config->get(db->config, tmp, 0);
    if (path == NULL ||
        (outf = fopen(path, "w")) == NULL)
    {
        INFO(INFO_ERROR, "Cannot open template file %s\n",
            path ? path : "<empty>");
        return DC_NOTOK;
    }

    for (t = dbdata->templates; t != NULL; t = t->next)
    {
        struct language_description *langdesc;
        INFO(INFO_VERBOSE, "dumping template %s\n", t->tag);

        fprintf(outf, "Name: %s\n", escapestr(t->tag));
        fprintf(outf, "Type: %s\n", escapestr(t->type));
        if (t->defaultval != NULL)
            fprintf(outf, "Default: %s\n", escapestr(t->defaultval));
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
    }

    if (fclose(outf) == EOF)
        perror("fclose");

    return DC_OK;
}

static struct template *rfc822db_template_get(struct template_db *db, 
    const char *ltag)
{
    struct template_db_cache *dbdata = db->data;
    struct template *t;
    t = find_template(dbdata->templates, ltag);
    if (t)
        template_ref(t);
    return t;
}

static int rfc822db_template_set(struct template_db *db, struct template *template)
{
    struct template *t = template_dup(template);
    struct template_db_cache *dbdata = db->data;
    struct template **pt;

    INFO(INFO_VERBOSE, "rfc822db_template_set(db,t=%s)\n", t->tag);

    for (pt = &dbdata->templates; *pt != NULL; pt = &(*pt)->next)
    {
        if (strcmp((*pt)->tag, t->tag) == 0)
        {
            /* one was there before, remove the old one, we'll
             * add one back later */
            struct template *tmp = *pt;

            *pt = tmp->next;
            template_deref(tmp);
            
            break;
        }
    }
    
    /* add it now (to the end) */
    pt = &dbdata->templates;
    while (*pt != NULL)
        pt = &(*pt)->next;

    *pt = t;

    return DC_OK;
}

static int rfc822db_template_remove(struct template_db *db, const char *tag)
{
    struct template_db_cache *dbdata = db->data;
    struct template **pt;

    INFO(INFO_VERBOSE, "rfc822db_template_remove(db,tag=%s)\n",tag);

    for (pt = &dbdata->templates; *pt != NULL; pt = &(*pt)->next)
    {
        if (strcmp((*pt)->tag, tag) == 0)
        {
            struct template **tmp = &(*pt)->next;

            template_deref(*pt);
            pt = tmp;

            return DC_OK;
        }
    }
    
    return DC_NOTOK;
}

static struct template *rfc822db_template_iterate(struct template_db *db, void **iter)
{
    INFO(INFO_VERBOSE, "rfc822db_template_iterate stub\n");
    return NULL;
}

/* config database */
static int rfc822db_question_initialize(struct question_db *db, struct configuration *cfg)
{
    struct question_db_cache *dbdata;
    /*    fprintf(stderr,"rfc822db_initialize(db,cfg)\n");*/
    dbdata = malloc(sizeof(struct question_db_cache));

    if (dbdata == NULL)
        return DC_NOTOK;

    dbdata->questions = NULL;
    db->data = dbdata;

    return DC_OK;
}

/*
 * Function: rfc822db_question_load
 * Input: question database
 * Output: DC_OK/DC_NOTOK
 * Description: parse a question db file and put it into the cache
 * Assumptions: the file is in valid rfc822 format
 * TODO: don't use linked lists, it's slow
 */
static int rfc822db_question_load(struct question_db *db)
{
    struct question_db_cache *dbdata = db->data;
    char tmp[1024];
    const char *path;
    FILE *inf;
    struct rfc822_header *header = NULL;
    struct question **questions;

    questions = &dbdata->questions;
    while (*questions != NULL)
        questions = &(*questions)->next;

    snprintf(tmp, sizeof(tmp), "%s::path", db->configpath);
    path = db->config->get(db->config, tmp, 0);
    if (path == NULL ||
        (inf = fopen(path, "r")) == NULL)
    {
        if (errno == ENOENT)
            return DC_NOTOK;

        INFO(INFO_ERROR, "Cannot open config database %s: %s\n",
            path ? path : "<empty>", strerror(errno));
        return DC_NOTOK;
    }

    while ((header = rfc822db_parse_stanza(inf)) != NULL)
    {
        struct question *tmp;
        const char *name;

        name = rfc822db_header_lookup(header, "name");

        if (name == NULL || *name == 0)
        {
            INFO(INFO_ERROR, "Read a stanza without a name\n");
            DELETE(header);
            continue;
        }

        tmp = question_new(name);

        question_setvalue(tmp, rfc822db_header_lookup(header, "value"));
        tmp->flags = parse_flags(rfc822db_header_lookup(header,"flags"));
        parse_owners(tmp, rfc822db_header_lookup(header, "owners"));
        parse_variables(tmp, rfc822db_header_lookup(header, "variables"));
        tmp->template = db->tdb->methods.get(db->tdb, rfc822db_header_lookup(header, "template"));

        *questions = tmp;
        questions = &tmp->next;

        DELETE(header);
    }

    fclose(inf);

    return DC_OK;
}

static int rfc822db_question_save(struct question_db *db)
{
    FILE *outf;
    struct question_db_cache *dbdata = db->data;
    char tmp[1024];
    const char *path;
    struct question *q;
    struct questionowner *owner;
	struct questionvariable *var;

    snprintf(tmp, sizeof(tmp), "%s::path", db->configpath);
    path = db->config->get(db->config, tmp, 0);
    if (path == NULL ||
        (outf = fopen(path, "w")) == NULL)
    {
        INFO(INFO_ERROR, "Cannot open question file %s\n",
            path ? path : "<empty>");
        return DC_NOTOK;
    }

    for (q = dbdata->questions; q != NULL; q = q->next)
    {
        INFO(INFO_VERBOSE, "dumping question %s\n", q->tag);
        fprintf(outf, "Name: %s\n", escapestr(q->tag));
        fprintf(outf, "Template: %s\n", escapestr(q->template->tag));
        if ((q->flags & DC_QFLAG_SEEN) || (q->value && *q->value != 0))
            fprintf(outf, "Value: %s\n", (q->value ? escapestr(q->value) : ""));

        if ((owner = q->owners))
        {
            fprintf(outf, "Owners: ");
            for (; owner != NULL; owner = owner->next)
            {
                fprintf(outf, "%s", escapestr(owner->owner));
                if (owner->next != NULL)
                    fprintf(outf, ", ");
            }
            fprintf(outf, "\n");
        }

        if (q->flags)
        {
            tmp[0] = 0;
            fprintf(outf, "Flags: ");

            /* TODO: handle multiple flags */
            if (q->flags & DC_QFLAG_SEEN)
                fprintf(outf, "seen");

            fprintf(outf, "\n");
        }
        
        if ((var = q->variables))
        {
            fprintf(outf, "Variables:\n");
            for (; var != NULL; var = var->next)
            {
                /* escapestr uses a static buf, so we do this in
                 * two steps */
                fprintf(outf, " %s = ", (var->variable ? escapestr(var->variable) : ""));
                fprintf(outf, "%s\n", (var->value ? escapestr(var->value) : ""));
            }
        }

        fprintf(outf, "\n");
    }

    if (fclose(outf) == EOF)
        perror("fclose");

    return DC_OK;
}

static struct question *rfc822db_question_get(struct question_db *db, 
    const char *ltag)
{
    struct question_db_cache *dbdata = db->data;
    struct question *q;

    q = find_question(dbdata->questions, ltag);
    if (q != NULL)
        question_ref(q);
    return q;
}

static int rfc822db_question_set(struct question_db *db, struct question *question)
{
    struct question *q = question_dup(question);
    struct question_db_cache *dbdata = db->data;
    /* the question in the db, if already there */
    struct question **pq;

    INFO(INFO_VERBOSE, "rfc822db_question_set(db,q=%s)\n", q->tag);

    for (pq = &dbdata->questions; *pq != NULL; pq = &(*pq)->next)
    {
        if (strcmp((*pq)->tag, q->tag) == 0)
        {
            /* one was there before, remove the old one, we'll
             * add one back later */
            struct question *tmp = *pq;

            *pq = tmp->next;
            question_deref(tmp);
            
            break;
        }
    }
    
    /* add it now (to the end) */
    pq = &dbdata->questions;
    while (*pq != NULL)
        pq = &(*pq)->next;

    *pq = q;

    return DC_OK;
}

static int rfc822db_question_disown(struct question_db *db, const char *tag, 
    const char *owner)
{
    struct question *q = rfc822db_question_get(db, tag);
    /*    fprintf(stderr,"rfc822db_question_disown(db, tag=%s, owner=%s)\n",
          tag,owner);*/
    if (q == NULL) 
        return DC_NOTOK;
    question_owner_delete(q, owner);
    rfc822db_question_set(db, q);
    question_deref(q);

    return DC_OK;
}

static struct question *rfc822db_question_iterate(struct question_db *db,
    void **iter)
{
    struct question_db_cache *dbdata;

    dbdata = db->data;

    if (*iter == NULL)
        *iter = dbdata->questions;
    else
        *iter = ((struct question *)*iter)->next;
      
    return (struct question *)*iter;
}

struct template_db_module debconf_template_db_module = {
    initialize: rfc822db_template_initialize,
    load: rfc822db_template_load,
    save: rfc822db_template_save,
    set: rfc822db_template_set,
    get: rfc822db_template_get,
    remove: rfc822db_template_remove,
    iterate: rfc822db_template_iterate,
};

struct question_db_module debconf_question_db_module = {
    initialize: rfc822db_question_initialize,
    load: rfc822db_question_load,
    save: rfc822db_question_save,
    set: rfc822db_question_set,
    get: rfc822db_question_get,
    disown: rfc822db_question_disown,
    iterate: rfc822db_question_iterate,
};

#if 0
int main(int argc, char** argv) 
{
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
#endif
