/* 
   debian-installer.c - common utilities for debian-installer
   Author - David Kimdon

   Copyright (C) 2000-2002  David Kimdon <dwhedon@debian.org>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <stdarg.h>
#include <time.h>
#include "debian-installer.h"



#define PREBASECONFIG_D "/usr/lib/prebaseconfig.d"

#ifdef L__di_prebaseconfig_append__
/*
   di_prebaseconfig_append()

   Append to a script in /usr/lib/prebaseconfig.d/.  All the scripts in this
   directory will be run immediately before the system is rebooted.  udebs drop
   commands in here that they know need to be run at this time.  
   
   udeb - Identifier, most likely the udeb making the call
   
   format, . . . - printf formatted text that will be appended to
   /usr/lib/prebaseconfig.d/<udeb>
*/
int
di_prebaseconfig_append(const char *udeb, const char *fmt, ...)
{
  char *path = NULL;
  FILE *fp = NULL;
  int rv = -1;
  va_list ap;
  time_t t;
  
  if (asprintf (&path, PREBASECONFIG_D "/%s", udeb) == -1) 
   {
    perror ("di_prebaseconfig_append: asprintf");
    goto finished;
   }
  
  if ( (fp = fopen (path, "a")) == NULL) 
   {
    perror ("di_prebaseconfig_append: fopen");
    goto finished;
   }
  
  time(&t);
  fprintf(fp, "\n# start entry %s\n", ctime(&t));
  
  va_start(ap, fmt);
  vfprintf(fp, fmt, ap);
  va_end(ap);
  
  fprintf(fp, "\n# end entry\n");
  
  rv = 0;
  
finished:
  free(path);
  if (fp)
    fclose(fp);
  return rv;
}

#endif /* L__di_prebaseconfig_append__ */


#ifdef L__di_execlog__
/* di_execlog():
   execute the command, log the result
   */

int
di_execlog (const char *incmd)
{
  FILE *output;
  char cmd[strlen (incmd) + 6];
  char line[MAXLINE];
  strcpy (cmd, incmd);

  openlog ("installer", LOG_PID | LOG_PERROR, LOG_USER);
  syslog (LOG_DEBUG, "running cmd '%s'", cmd);

  /* FIXME: this can cause the shell command if there's redirection
     already in the passed string */
  strcat (cmd, " 2>&1");
  output = popen (cmd, "r");
  while (fgets (line, MAXLINE, output) != NULL)
    {
      syslog (LOG_DEBUG, line);
    }
  closelog ();
  /* FIXME we aren't getting the return value from the actual command
     executed, not sure how to do that cleanly */
  return (pclose (output));
}
#endif /* L__di_execlog__ */

#ifdef L__di_log__

void
di_log(const char *msg){
    openlog ("installer", LOG_PID | LOG_PERROR, LOG_USER);
    syslog (LOG_DEBUG, "%s", msg);
    closelog ();
}

#endif /* L__di_log__ */


#ifdef L__di_check_dir__
int
di_check_dir (const char *dirname)
{
  struct stat check;
  if (stat (dirname, &check) == -1)
    return -1;
  else
    return S_ISDIR (check.st_mode);
}

#endif /*  L__di_check_dir__ */

#ifdef L__di_snprintfcat__
int
di_snprintfcat (char *str, size_t size, const char *format, ...)
{
  va_list ap;
  int retval;
  size_t len = strlen (str);

  va_start (ap, format);
  retval = vsnprintf (str + len, size - len, format, ap);
  va_end (ap);

  return retval;
}
#endif /* L__di_snprintfcat__ */

#ifdef L__di_stristr__
char *
di_stristr(const char *haystack, const char *needle)
{
    /* I suppose using strncasecmp is suboptimal, but we are already
     * using GNU extensions in other places. */
    const char *ret;
    size_t n_len;

    n_len = strlen(needle);
    for (ret = haystack; *ret != '\0'; ret++)
    {
        if (strncasecmp(ret, needle, n_len) == 0)
            return (char *)ret;
    }
    return NULL;
}
#endif /* L__di_stristr__ */

#ifdef L__di_pkg_parse__
#define BUFSIZE         4096
struct package_t *
di_pkg_parse(FILE *f)
{
    char buf[BUFSIZE];
    struct package_t *p = NULL, *newp;
    char *b;
    int i;

    while (fgets(buf, BUFSIZE, f) && !feof(f))
    {
        buf[strlen(buf)-1] = 0;
        if (di_stristr(buf, "Package: ") == buf)
        {
            newp = (struct package_t *)malloc(sizeof(struct package_t));
            memset(newp, 0, sizeof(struct package_t));
            newp->package = strdup(strchr(buf, ' ') + 1);
            newp->next = p;
            p = newp;
        }
        else if (di_stristr(buf, "Filename: ") == buf)
        {
            p->filename = strdup(strchr(buf, ' ') + 1);
        }
        else if (di_stristr(buf, "MD5sum: ") == buf)
        {
            p->md5sum = strdup(strchr(buf, ' ') + 1);
        }
        else if (di_stristr(buf, "installer-menu-item: ") == buf)
        {
            p->installer_menu_item = atoi(strchr(buf, ' ') + 1);
        }
        else if (di_stristr(buf, "Status: ") == buf)
        {
            if (di_stristr(buf, " unpacked"))
                p->status = unpacked;
            else if (di_stristr(buf, " half-configured"))
                p->status = half_configured;
            else if (di_stristr(buf, " installed"))
                p->status = installed;
            else
                p->status = other;
        }
        else if (di_stristr(buf, "Description: ") == buf)
        {
            p->description = strdup(strchr(buf, ' ') + 1);
        }
        else if (di_stristr(buf, "Description-") == buf)
        {
            struct language_description *langdesc;
            char *colon_idx, *dash_idx;
            char *lang_code;
            int lang_code_len;
            
            dash_idx = strchr(buf, '-');
            if ((colon_idx = strstr(buf, ": ")) != NULL)
            {
                lang_code_len = colon_idx - dash_idx;
                lang_code = (char *)malloc(lang_code_len);
                memcpy(lang_code, dash_idx + 1, lang_code_len-1);
                lang_code[lang_code_len-1] = '\0';
                langdesc = malloc(sizeof(struct language_description));
                memset(langdesc, 0, sizeof(struct language_description));
                langdesc->language = lang_code;
                langdesc->description = strdup(colon_idx + 2);
                if (p->localized_descriptions) 
                    langdesc->next = p->localized_descriptions;
                p->localized_descriptions = langdesc;
            }
        }
        else if (di_stristr(buf, "Depends: ") == buf)
        {
            /*
             * Basic depends line parser. Can ignore versioning
             * info since the depends are already satisfied.
             */
            b = strdup(strchr(buf, ' ') + 1);
            i = 0;
            while (*b != 0 && *b != '\n')
            {
                if (*b != ' ')
                {
                    if (*b == ',')
                    {
                        *b = 0;
                        p->depends[++i] = 0;
                    }
                    else if (p->depends[i] == 0)
                        p->depends[i] = b;
                }
                else
                    *b = 0; /* eat the space... */
                b++;
            }
            *b = 0;
            p->depends[i+1] = 0;
        }
        else if (di_stristr(buf, "Provides: ") == buf)
        {
            p->provides = strdup(strchr(buf, ' ') + 1);
        }
    }

    return p;
}
#endif /* L__di_pkg_parse__ */
