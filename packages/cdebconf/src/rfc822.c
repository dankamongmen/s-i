#include <stdio.h>
#include <ctype.h>

#include "common.h"
#include "rfc822.h"
#include "strutl.h"


static char *unescapestr(const char *in)
{
    static char buf[8192];
    if (in == 0) return 0;
    strunescape(in, buf, sizeof(buf), 0);
    return buf;
}

/*
 * Function: rfc822db_parse_stanza
 * Input: a FILE pointer to an open readable file containing a stanza in rfc822 
 *    format.
 * Output: a pointer to a dynamically allocated rfc822_header structure
 * Description: parse a stanza from file into the returned header struct
 * Assumptions: no lines are over 8192 bytes long.
 */

struct rfc822_header* rfc822_parse_stanza(FILE *file)
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
            *tmp++ = '\0';

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


char *rfc822_header_lookup(struct rfc822_header *list, const char* key)
{
/*    fprintf(stderr,"rfc822db_header_lookup(list,key=%s)\n",key);*/
    while (list && (strcasecmp(key, list->header) != 0))
        list = list->next;
    if (!list)
        return NULL;
/*    fprintf(stderr,"rfc822db_header_lookup returning: '%s'\n", list->value);*/
    return list->value;
}
