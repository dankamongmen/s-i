#include "common.h"
#include "configuration.h"
#include "database.h"
#include "rfc822.h"
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
#include <string.h>
#include <search.h>

FILE *outf = NULL;

int nodetemplatecomp(const void *pa, const void *pb) {
  return strcmp(((struct template *)pa)->tag, 
                ((struct template *)pb)->tag);
}

int nodequestioncomp(const void *pa, const void *pb) {
  return strcmp(((struct question *)pa)->tag, 
                ((struct question *)pb)->tag);
}


static char *unescapestr(const char *in)
{
    static char buf[8192];
    if (in == 0) return 0;
    strunescape(in, buf, sizeof(buf), 0);
    return buf;
}

static char *escapestr(const char *in)
{
    static char buf[8192];
    if (in == 0) return 0;
    strescape(in, buf, sizeof(buf), 0);
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
        if (finished != 0)
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
   
    if (!string)
	    return;

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
        if (finished != 0)
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

/* templates */
static int rfc822db_template_initialize(struct template_db *db, struct configuration *cfg)
{
    struct template_db_cache *dbdata;
    /*    fprintf(stderr,"rfc822db_initialize(db,cfg)\n");*/
    dbdata = malloc(sizeof(struct template_db_cache));
    if (dbdata == NULL)
        return DC_NOTOK;

    dbdata->root = NULL;
    db->data = dbdata;

    return DC_OK;
}

/*
 * Function: rfc822db_template_load
 * Input: template database
 * Output: DC_OK/DC_NOTOK
 * Description: parse a template db file and put it into the cache
 * Assumptions: the file is in valid rfc822 format
 */
static int rfc822db_template_load(struct template_db *db)
{
    struct template_db_cache *dbdata = db->data;
    char tmp[1024];
    const char *path;
    FILE *inf;
    struct rfc822_header *header = NULL;
    snprintf(tmp, sizeof(tmp), "%s::path", db->configpath);
    path = db->config->get(db->config, tmp, 0);
    if (path == NULL ||
        (inf = fopen(path, "r")) == NULL)
    {
        INFO(INFO_VERBOSE, "Cannot open template file %s\n",
            path ? path : "<empty>");
        return DC_NOTOK;
    }

    while ((header = rfc822_parse_stanza(inf)) != NULL)
    {
        struct template *tmp;
        const char *name;
        struct rfc822_header *h;

        name = rfc822_header_lookup(header, "name");
        if (name == NULL)
        {
            INFO(INFO_ERROR, "Read a stanza without a name\n");
            DELETE(header);
            continue;
        }

        tmp = template_new(name);
        for (h = header; h != NULL; h = h->next)
            if (strcmp(h->header, "Name") != 0)
                tmp->lset(tmp, NULL, h->header, h->value);

        tmp->next = NULL;
        tsearch(tmp, &dbdata->root, nodetemplatecomp);
    }

    fclose(inf);

    return DC_OK;
}

void rfc822db_template_dump(const void *node, const VISIT which, const int depth)
{
    const char *p, *lang;
    const char **field;
    const struct template *t = (*(struct template **) node);

    switch (which) {
    case preorder:
        break;
    case endorder:
        break;
    case postorder: 
    case leaf:
        p = t->lget((struct template *) t, NULL, "tag");
        INFO(INFO_VERBOSE, "dumping template %s\n", p);

        for (field = template_fields_list; *field != NULL; field++)
        {
            p = t->lget((struct template *) t, NULL, *field);
            if (p != NULL)
            {
                if (strcmp(*field, "tag") == 0)
                    fprintf(outf, "Name: %s\n", escapestr(p));
                else
                    fprintf(outf, "%c%s: %s\n",
                        toupper((*field)[0]), (*field)+1, escapestr(p));
            }
        }
    
        lang = t->next_lang((struct template *) t, NULL);
        while (lang) 
        {
            for (field = template_fields_list; *field != NULL; field++)
            {
                p = t->lget((struct template *) t, lang, *field);
                if (p != NULL && p != t->lget((struct template *) t, NULL, *field))
                    fprintf(outf, "%c%s-%s.UTF-8: %s\n",
                        toupper((*field)[0]), (*field)+1, lang, escapestr(p));
            }
            lang = t->next_lang((struct template *) t, lang);
        }
        fprintf(outf, "\n");
    }
}

static int rfc822db_template_save(struct template_db *db)
{
    struct template_db_cache *dbdata = db->data;
    char tmp[1024];
    const char *path;

    if (outf != NULL)
    {
            INFO(INFO_ERROR, "Internal incostisency error, outf is not NULL");
            return DC_NOTOK;
    }

    snprintf(tmp, sizeof(tmp), "%s::path", db->configpath);
    path = db->config->get(db->config, tmp, 0);
    if (path == NULL)
    {
        INFO(INFO_ERROR, "Cannot open template file <empty>\n");
        return DC_NOTOK;
    }
    else if ((outf = fopen(path, "w")) == NULL)
    {
        INFO(INFO_ERROR, "Cannot open template file %s: %s\n",
            path, strerror(errno));
        return DC_NOTOK;
    }

    twalk(dbdata->root, rfc822db_template_dump);

    if (fclose(outf) == EOF)
        perror("fclose");
    outf = NULL;
    return DC_OK;
}

static struct template *rfc822db_template_get(struct template_db *db, 
    const char *ltag)
{
        struct template_db_cache *dbdata = db->data;
        struct template *t, t2;
        t2.tag = (char*) ltag; /* Get rid of warning from gcc -- 
                                  this should be safe */
        t = tfind(&t2, &dbdata->root, nodetemplatecomp);
        if (t)
        {
                t = (*(struct template **) t);
                template_ref(t);
        }

        return t;
}

static int rfc822db_template_set(struct template_db *db, struct template *template)
{
        struct template_db_cache *dbdata = db->data;
    INFO(INFO_VERBOSE, "rfc822db_template_set(db,t=%s)\n", template->tag);

    tdelete(template, &dbdata->root, nodetemplatecomp);
    tsearch(template, &dbdata->root, nodetemplatecomp);
   
    template_ref(template);

    return DC_OK;
}

/* FIXME b0rken */
static int rfc822db_template_remove(struct template_db *db, const char *tag)
{
    struct template_db_cache *dbdata = db->data;
    struct template *t, t2;

    INFO(INFO_VERBOSE, "rfc822db_template_remove(db,tag=%s)\n",tag);

    t2.tag = (char*) tag;
    t = tdelete(&t, &dbdata->root, nodetemplatecomp);

    if (t)
    {
            template_deref(t);
            return DC_OK;
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

    dbdata->root = NULL;
    db->data = dbdata;

    return DC_OK;
}

/*
 * Function: rfc822db_question_load
 * Input: question database
 * Output: DC_OK/DC_NOTOK
 * Description: parse a question db file and put it into the cache
 * Assumptions: the file is in valid rfc822 format
 */
static int rfc822db_question_load(struct question_db *db)
{
    struct question_db_cache *dbdata = db->data;
    char tmp[1024];
    const char *path;
    FILE *inf;
    struct rfc822_header *header = NULL;

    snprintf(tmp, sizeof(tmp), "%s::path", db->configpath);
    path = db->config->get(db->config, tmp, 0);
    if (path == NULL ||
        (inf = fopen(path, "r")) == NULL)
    {
        if (errno == ENOENT)
            return DC_NOTOK;

        INFO(INFO_VERBOSE, "Cannot open config database %s: %s\n",
            path ? path : "<empty>", strerror(errno));
        return DC_NOTOK;
    }

    while ((header = rfc822_parse_stanza(inf)) != NULL)
    {
        struct question *tmp;
        const char *name;

        name = rfc822_header_lookup(header, "name");

        if (name == NULL || *name == 0)
        {
            INFO(INFO_ERROR, "Read a stanza without a name\n");
            DELETE(header);
            continue;
        }

        tmp = question_new(name);

        question_setvalue(tmp, rfc822_header_lookup(header, "value"));
        tmp->flags = parse_flags(rfc822_header_lookup(header,"flags"));
        parse_owners(tmp, rfc822_header_lookup(header, "owners"));
        parse_variables(tmp, rfc822_header_lookup(header, "variables"));
        tmp->template = db->tdb->methods.get(db->tdb, rfc822_header_lookup(header, "template"));
        if (tmp->template == NULL) {
                tmp->template = template_new(name);
                db->tdb->methods.set(db->tdb, tmp->template);
        }
        tsearch(tmp, &dbdata->root, nodequestioncomp);
        DELETE(header);
    }

    fclose(inf);

    return DC_OK;
}

void rfc822db_question_dump(const void *node, const VISIT which, const int depth)
{
  struct questionowner *owner;
  struct questionvariable *var;
  char tmp[1024];

  const struct question *q = (*(struct question **) node);
  switch (which) {
  case preorder:
    break;
  case endorder:
    break;
  case postorder: 
  case leaf:

        INFO(INFO_VERBOSE, "dumping question %s\n", (q)->tag); 
        fprintf(outf, "Name: %s\n", escapestr((q)->tag));
        fprintf(outf, "Template: %s\n", escapestr((q)->template->tag));
        if (((q)->flags & DC_QFLAG_SEEN) || (q)->value)
            fprintf(outf, "Value: %s\n", ((q)->value ? escapestr((q)->value) : ""));

        if ((owner = (q)->owners))
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

        if ((q)->flags)
        {
            tmp[0] = 0;
            fprintf(outf, "Flags: ");

            /* TODO: handle multiple flags */
            if ((q)->flags & DC_QFLAG_SEEN)
                fprintf(outf, "seen");

            fprintf(outf, "\n");
        }
        
        if ((var = (q)->variables))
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
}


static int rfc822db_question_save(struct question_db *db)
{
    struct question_db_cache *dbdata = db->data;
    const char *path;
    char tmp[1024];
    
    snprintf(tmp, sizeof(tmp), "%s::path", db->configpath);
    path = db->config->get(db->config, tmp, 0);
    if (path == NULL)
    {
        INFO(INFO_ERROR, "Cannot open question file <empty>\n");
        return DC_NOTOK;
    }
    else if ((outf = fopen(path, "w")) == NULL)
    {
        INFO(INFO_ERROR, "Cannot open question file %s: %s\n",
            path, strerror(errno));
        return DC_NOTOK;
    }

    twalk(dbdata->root, rfc822db_question_dump);

    if (fclose(outf) == EOF)
        perror("fclose");
    outf = NULL;

    return DC_OK;
}

static struct question *rfc822db_question_get(struct question_db *db, 
    const char *ltag)
{
    struct question_db_cache *dbdata = db->data;
    struct question *q, q2;

    memset(&q2, 0, sizeof (struct question));
    q2.tag = strdup(ltag);
    q = tfind(&q2, &dbdata->root, nodequestioncomp);
    free(q2.tag);
    if (q != NULL)
    {
            q = *(struct question **)q;
            question_ref(q);
    }

    return q;
}

static int rfc822db_question_set(struct question_db *db, struct question *question)
{
    struct question_db_cache *dbdata = db->data;
    struct question *q;

    INFO(INFO_VERBOSE, "rfc822db_question_set(db,q=%s,q=%p)\n", question->tag, question);

    tdelete(question, &dbdata->root, nodequestioncomp);
    q = tsearch(question, &dbdata->root, nodequestioncomp);
    question_ref(question);

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
        INFO(INFO_VERBOSE, "rfc822db_question_iterate stub\n");
        return NULL;
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

void dump_question(struct question *q) {
    fprintf(stderr,"\nDUMPING QUESTION\n");
    fprintf(stderr,"Question: %s\n", q->tag);
    fprintf(stderr,"Value: %s\n", q->value);
    fprintf(stderr,"Ref count: %d\n", q->ref);
    fprintf(stderr,"Flags: %d\n", q->flags);
    fprintf(stderr,"Template: %p", q->template);
    fprintf(stderr,"(%s)\n", q->template->tag);
}

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
