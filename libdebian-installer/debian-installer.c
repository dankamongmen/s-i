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

        if (asprintf (&path, PREBASECONFIG_D "/%s", udeb) == -1) {
                perror ("di_prebaseconfig_append: asprintf");
                goto finished;
        }
        
        if ( (fp = fopen (path, "a")) == NULL) {
                perror ("di_prebaseconfig_append: fopen");
                goto finished;
        }

        time(&t);
        fprintf(fp, "\n# start entry %s\n", ctime(&t));

        va_start(ap, fmt);
        fprintf(fp, fmt, ap);
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
di_log(char *msg){
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
