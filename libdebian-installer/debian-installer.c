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
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include "debian-installer.h"



#define PREBASECONFIG_D "/usr/lib/prebaseconfig.d"

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



/* di_execlog():
   execute the command, log the result
   */

int
di_execlog (const char *incmd)
{
  char cmd[strlen (incmd) + 6];
  char line[MAXLINE];
  pid_t pid;
  FILE *outputf;
  int output[2], status = -1;

  openlog ("installer", LOG_PID, LOG_USER);

  pipe (output);

  snprintf (cmd, strlen (incmd) + 6, "exec %s", incmd);

  pid = fork ();
  if (pid == 0)
  {
    int i;

    dup2 (output[1], 1);
    dup2 (output[1], 2);
    close (output[0]);
    close (output[1]);

    i = open ("/dev/null", O_RDONLY);
    dup2 (i, 0);
    close (i);

    execl ("/bin/sh", "sh", "-c", cmd, NULL);
    exit (127);
  }
  else if (pid < 0)
  {
    close (output[1]);
    close (output[0]);
    syslog (LOG_DEBUG, "running cmd '%s' failed", incmd);
  }
  else
  {
    int n;

    close (output[1]);
    outputf = fdopen (output[0], "r");

    syslog (LOG_DEBUG, "running cmd '%s'", incmd);

    while (fgets (line, MAXLINE, outputf) != NULL)
      syslog (LOG_DEBUG, line);

    fclose (outputf); /* closes output[0] */

    do
      n = waitpid (pid, &status, WNOHANG);
    while (n == -1 && errno == EINTR);

    if (n != pid)
      status = -1;
  }

  closelog ();

  return WIFEXITED(status) ? WEXITSTATUS(status) : status;
}


void
di_log(const char *msg){
  di_logf("%s", msg);
}



void
di_logf(const char *fmt, ...){
  va_list ap;
  openlog ("installer", LOG_PID, LOG_USER);
  va_start(ap, fmt);
  vsyslog (LOG_DEBUG, fmt, ap);
  va_end(ap);
  closelog ();
}



  int
di_check_dir (const char *dirname)
{
  struct stat check;
  if (stat (dirname, &check) == -1)
    return -1;
  else
    return S_ISDIR (check.st_mode);
}


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

  struct package_t *
di_pkg_alloc(const char *name)
{
  struct package_t *p;

  p = malloc(sizeof(struct package_t));
  memset(p, 0, sizeof(struct package_t));
  p->package = strdup(name);
  return p;
}

  void
di_pkg_free(struct package_t *p)
{
  struct language_description *l1, *l2;
  int i;

  /* No-op if p is NULL, like free(3) */
  if (p == NULL)
    return;
  free(p->package);
  free(p->filename);
  free(p->md5sum);
  free(p->description);
  for (i = 0; p->depends[i] != NULL; i++)
  {
    free(p->depends[i]->name);
    free(p->depends[i]);
  }
  for (i = 0; p->provides[i] != NULL; i++)
  {
    free(p->provides[i]->name);
    free(p->provides[i]);
  }
  l1 = p->localized_descriptions;
  while (l1 != NULL)
  {
    l2 = l1;
    l1 = l1->next;
    free(l2->language);
    free(l2->description);
    free(l2);
  }
  free(p->version);
  free(p);
}

  static char *
parse_dependency(char *buf, struct package_dependency **dep)
{
  char *b;

  /*
   * Basic depends line parser. Can ignore versioning
   * info since the depends are already satisfied.
   */
  while (*buf != 0 && *buf != '\n')
  {
    if (*buf != ' ' && *buf != ',')
    {
      b = buf;
      while (*b != 0 && *b != '\n' && *b != ',')
      {
        if (*b == ' ')
          *b = 0;
        b++;
      }
      *b = 0;
      *dep = malloc(sizeof(struct package_dependency));
      (*dep)->name = strdup(buf);
      (*dep)->ptr = NULL;
      return b+1;
    }
    buf++;
  }
  return NULL;
}

#define BUFSIZE         4096
struct linkedlist_t *di_pkg_parse(FILE *f)
{
  char buf[BUFSIZE];
  struct linkedlist_t *list;
  struct list_node *node;
  struct package_t *p = NULL;
  char *b;
  int i;

  list = malloc(sizeof(struct linkedlist_t));
  list->head = list->tail = NULL;
  while (fgets(buf, BUFSIZE, f) && !feof(f))
  {
    buf[strlen(buf)-1] = 0;
    if (di_stristr(buf, "Package: ") == buf)
    {
      p = di_pkg_alloc(strchr(buf, ' ') + 1);
      node = malloc(sizeof(struct list_node));
      node->next = NULL;
      node->data = p;
      if (list->head != NULL)
      {
        list->tail->next = node;
        list->tail = node;
      }
      else
      {
        list->head = node;
        list->tail = node;
      }
    }
    else if (di_stristr(buf, "Version: ") == buf)
    {
      p->version = strdup(strchr(buf, ' ') + 1);
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
    else if (di_stristr(buf, "Priority: ") == buf)
    {
      if (di_stristr(buf, " extra"))
        p->priority = extra;
      else if (di_stristr(buf, " optional"))
        p->priority = optional;
      else if (di_stristr(buf, " standard"))
        p->priority = standard;
      else if (di_stristr(buf, " important"))
        p->priority = important;
      else if (di_stristr(buf, " required"))
        p->priority = required;
      else
        /* ??? */
        p->priority = extra;
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
        lang_code = malloc(lang_code_len);
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
      b = strchr(buf, ' ')+1;
      i = 0;
      while (b != NULL) {
        b = parse_dependency(b, &p->depends[i++]);
        if (i == DEPENDSMAX)
        {
          char *emsg;

          asprintf(&emsg, "Package %s has more than %d dependencies!",
              p->package, DEPENDSMAX-1);
          di_log(emsg);
          free(emsg);
          break;
        }
      }
      p->depends[i] = NULL;
    }
    else if (di_stristr(buf, "Provides: ") == buf)
    {
      b = strchr(buf, ' ')+1;
      i = 0;
      while (b != NULL) {
        b = parse_dependency(b, &p->provides[i++]);
        if (i == PROVIDESMAX)
        {
          char *emsg;

          asprintf(&emsg, "Package %s has more than %d provides!",
              p->package, PROVIDESMAX-1);
          di_log(emsg);
          free(emsg);
          break;
        }
      }
      p->provides[i] = NULL;
    }
    else if (di_stristr(buf, "Recommends: ") == buf)
    {
      b = strchr(buf, ' ')+1;
      i = 0;
      while (b != NULL) {
        b = parse_dependency(b, &p->recommends[i++]);
        if (i == RECOMMENDSMAX)
        {
          char *emsg;

          asprintf(&emsg, "Package %s has more than %d recommends!",
              p->package, RECOMMENDSMAX-1);
          di_log(emsg);
          free(emsg);
          break;
        }
      }
      p->recommends[i] = NULL;
    }
    else if (di_stristr(buf, "Enhances: ") == buf)
    {
      b = strchr(buf, ' ')+1;
      i = 0;
      while (b != NULL) {
        b = parse_dependency(b, &p->enhances[i++]);
        if (i == ENHANCESMAX)
        {
          char *emsg;

          asprintf(&emsg, "Package %s has more than %d enhances!",
              p->package, ENHANCESMAX-1);
          di_log(emsg);
          free(emsg);
          break;
        }
      }
      p->enhances[i] = NULL;
    }
  }

  return list;
}

/*
 * Read status file and generate and return a linked list of packages.
 */
struct linkedlist_t *di_status_read(void) {
	FILE *f;
	struct linkedlist_t *plist;

	if ((f = fopen(STATUSFILE, "r")) == NULL) {
		perror(STATUSFILE);
		return 0;
	}
	plist = di_pkg_parse(f);
	fclose(f);
	di_pkg_resolve_deps(plist);

	return plist;
}

  struct package_t *
di_pkg_find(struct linkedlist_t *ptr, const char *package)
{
  struct list_node *node;
  struct package_t *p;

  for (node = ptr->head; node != NULL; node = node->next)
  {
    p = (struct package_t *)node->data;
    if (strcmp(p->package, package) == 0)
      return p;
  }
  return NULL;
}

/* Does 'ptr' provide 'target'? */
  int
di_pkg_provides(struct package_t *p, struct package_t *target)
{
  int i;

  for (i = 0; p->provides[i]; i++) {
    if (p->provides[i]->ptr == target)
      return 1;
  }
  return 0;
}

  int
di_pkg_is_virtual(struct package_t *p)
{
  int i;

  for (i = 0; p->depends[i] != NULL; i++)
  {
    if (p->depends[i]->ptr != NULL && di_pkg_provides(p->depends[i]->ptr, p))
      return 1;
  }
  return 0;
}

/* For a virtual package, it's true if any of its providing packages
 * is installed */
  int
di_pkg_is_installed(struct package_t *p)
{
  struct package_t *q;
  int i;

  for (i = 0; p->depends[i] != NULL; i++)
  {
    q = p->depends[i]->ptr;
    if (q != NULL && di_pkg_provides(q, p) && q->status == installed)
      return 1;
  }
  return (p->status == installed);
}

/* Resolve dependency/provides pointers. Creates dummy packages for virtual
 * packages with reverse depends.
 *
 * This function has quadratic complexity over the number of packages,
 * but since the number of packages will typically be <100, we don't
 * care all that much about it. */
  void
di_pkg_resolve_deps(struct linkedlist_t *ptr)
{
  struct list_node *node, *newnode;
  struct package_t *p, *q;
  struct package_dependency *dep;
  int i, j;

  /* Start by resolving provides, creating bogus packages with reverse depends */
  for (node = ptr->head; node != NULL; node = node->next)
  {
    p = (struct package_t *)node->data;
    for (i = 0; p->provides[i] != NULL; i++)
    {
      if ((q = di_pkg_find(ptr, p->provides[i]->name)) == NULL)
      {
        q = di_pkg_alloc(p->provides[i]->name);
        newnode = malloc(sizeof(struct list_node));
        newnode->data = q;
        newnode->next = ptr->head;
        ptr->head = newnode;
      }
      p->provides[i]->ptr = q;
      for (j = 0; q->depends[j] != NULL; j++)
        ;
      dep = malloc(sizeof(struct package_dependency));
      dep->name = strdup(p->package);
      dep->ptr = p;
      q->depends[j] = dep;
      q->depends[j+1] = NULL;
    }
  }

  /* Now resolve the dependencies */
  for (node = ptr->head; node != NULL; node = node->next)
  {
    p = (struct package_t *)node->data;
    for (i = 0; p->depends[i] != NULL; i++)
    {
      if (p->depends[i]->ptr == NULL)
        p->depends[i]->ptr = di_pkg_find(ptr, p->depends[i]->name);
    }
  }

  return;
}

/* visit function for the depth-first traversal performed by the
 * toposort functions */
  static void
dfs_visit(struct package_t *p, struct linkedlist_t *queue)
{
  struct package_t *q;
  struct list_node *node;
  int i;

  p->processed = 1;
  for (i = 0; p->depends[i] != NULL; i++) {
    q = p->depends[i]->ptr;
    if (q == NULL)
      continue;
    if (!q->processed)
      dfs_visit(q, queue);
  }
  for (i = 0; p->recommends[i] != NULL; i++) {
    q = p->recommends[i]->ptr;
    if (q == NULL)
      continue;
    if (!q->processed)
      dfs_visit(q, queue);
  }
  node = malloc(sizeof(struct list_node));
  node->data = p;
  node->next = NULL;
  if (queue->tail == NULL)
    queue->head = queue->tail = node;
  else {
    queue->tail->next = node;
    queue->tail = node;
  }
}

/* Create a topological ordering of set of packages. This will also pull in
 * their dependencies even if they're not actually in the list, because that's
 * what we want most of the time. By using a queue, we'll get the dependencies
 * before the packages that depend on them. */
  struct linkedlist_t *
di_pkg_toposort_arr(struct package_t **packages, const int pkg_count)
{
  struct linkedlist_t *queue;
  int i;

  queue = malloc(sizeof(struct linkedlist_t));
  queue->head = queue->tail = NULL;
  for (i = 0; i < pkg_count; i++)
    packages[i]->processed = 0;
  for (i = 0; i < pkg_count; i++)
    if (!packages[i]->processed)
      dfs_visit(packages[i], queue);
  return queue;
}

/* Same as di_pkg_toposort_arr, but for a linked list of packages */
  struct linkedlist_t *
di_pkg_toposort_list(struct linkedlist_t *list)
{
  struct linkedlist_t *ret;
  struct list_node *node;
  struct package_t **packages, *p;
  int count = 0, i = 0;

  for (node = list->head; node != NULL; node = node->next) {
    p = (struct package_t *)node->data;
    if (p != NULL)
      count++;
  }
  packages = malloc(count * sizeof(struct package_t *));
  for (node = list->head; node != NULL; node = node->next) {
    p = (struct package_t *)node->data;
    if (p != NULL)
      packages[i++] = p;
  }
  ret = di_pkg_toposort_arr(packages, count);
  free(packages);
  return ret;
}


/* Version parsing and comparison is taken from dpkg 1.10.9 */

/* assume ascii; warning: evaluates x multiple times! */
#define order(x) ((x) == '~' ? -1 \
    : isdigit((x)) ? 0 \
    : !(x) ? 0 \
    : isalpha((x)) ? (x) \
    : (x) + 256)

  static int
verrevcmp(const char *val, const char *ref)
{
  if (!val)
    val = "";
  if (!ref)
    ref = "";

  while (*val || *ref) {
    int first_diff = 0;

    while ((*val && !isdigit(*val)) || (*ref && !isdigit(*ref))) {
      int vc = order(*val), rc = order(*ref);
      if (vc != rc)
        return vc - rc;
      val++;
      ref++;
    }

    while (*val == '0')
      val++;
    while (*ref == '0')
      ref++;
    while (isdigit(*val) && isdigit(*ref)) {
      if (!first_diff)
        first_diff = *val - *ref;
      val++;
      ref++;
    }
    if (isdigit(*val))
      return 1;
    if (isdigit(*ref))
      return -1;
    if (first_diff)
      return first_diff;
  }
  return 0;
}


  int
di_parse_version(struct version_t *rversion, const char *string)
{
  char *hyphen, *colon, *eepochcolon;
  const char *end, *ptr;
  unsigned long epoch;

  if (!*string)
    return 0;

  /* trim leading and trailing space */
  while (*string && (*string == ' ' || *string == '\t'))
    string++;
  /* string now points to the first non-whitespace char */
  end = string;
  /* find either the end of the string, or a whitespace char */
  while (*end && *end != ' ' && *end != '\t')
    end++;
  /* check for extra chars after trailing space */
  ptr = end;
  while (*ptr && (*ptr == ' ' || *ptr == '\t'))
    ptr++;
  if (*ptr)
    return 0;

  colon = strchr(string,':');
  if (colon) {
    epoch = strtoul(string, &eepochcolon, 10);
    if (colon != eepochcolon)
      return 0;
    if (!*++colon)
      return 0;
    string = colon;
    rversion->epoch = epoch;
  } else {
    rversion->epoch = 0;
  }
  rversion->version = strndup(string, end-string);
  hyphen = strrchr(rversion->version, '-');
  if (hyphen)
    *hyphen++ = 0;
  rversion->revision = hyphen ? hyphen : "";

  return 1;
}


  int
di_compare_version(const struct version_t *a, const struct version_t *b)
{
  int r;

  if (a->epoch > b->epoch)
    return 1;
  if (a->epoch < b->epoch)
    return -1;
  r = verrevcmp(a->version, b->version);
  if (r != 0)
    return r;
  return verrevcmp(a->revision, b->revision);
}

  void
di_list_free(struct linkedlist_t *list, void (*freefunc)(void *))
{
  struct list_node *node, *next;

  if (list == NULL)
    return;
  for (node = list->head; node != NULL; node = next)
  {
    next = node->next;
    freefunc(node->data);
    free(node);
  }
  free(list);
}

/* Returns the devfs path name normalized into a "normal" (hdaX, sdaX)
 * name.  The return value is malloced, which means that the caller
 * needs to free it.  This is kinda hairy, but I don't see any other
 * way of operating with non-devfs systems when we use devfs on the
 * boot medium.  The numbers are taken from devices.txt in the
 * Documentation subdirectory off the kernel sources.
 */
  ssize_t
di_mapdevfs(const char *path, char *buf, size_t n)
{
  static struct entry
  {
    unsigned char major;
    unsigned char minor;
    char *name;
    enum { ENTRY_TYPE_ONE, ENTRY_TYPE_NUMBER, ENTRY_TYPE_DISC } type;
    unsigned char entry_first;
    unsigned char entry_disc_minor_shift;
  }
  entries[] =
  {
    /*
       major	minor	name		type			entry_first
       entry_disc_minor_shift */
    { 2,	0,	"fd",		ENTRY_TYPE_NUMBER,	0,	0 },
    { 3,	0,	"hd",		ENTRY_TYPE_DISC,	0,	6 },
    { 4,	64,	"ttyS",		ENTRY_TYPE_NUMBER,	0,	6 },
    { 4,	0,	"tty",		ENTRY_TYPE_NUMBER,	0,	6 },
    { 8,	0,	"sd",		ENTRY_TYPE_DISC,	0,	4 },
    { 9,    0,      "md",           ENTRY_TYPE_NUMBER,      0,      0 },
    { 22,	0,	"hd",		ENTRY_TYPE_DISC,	2,	6 },
    { 33,	0,	"hd",		ENTRY_TYPE_DISC,	4,	6 },
    { 34,	0,	"hd",		ENTRY_TYPE_DISC,	6,	6 },
    { 56,	0,	"hd",		ENTRY_TYPE_DISC,	8,	6 },
    { 57,	0,	"hd",		ENTRY_TYPE_DISC,	10,	6 },
    { 65,	0,	"sd",		ENTRY_TYPE_DISC,	16,	4 },
    { 66,	0,	"sd",		ENTRY_TYPE_DISC,	32,	4 },
    { 67,	0,	"sd",		ENTRY_TYPE_DISC,	48,	4 },
    { 68,	0,	"sd",		ENTRY_TYPE_DISC,	64,	4 },
    { 69,	0,	"sd",		ENTRY_TYPE_DISC,	80,	4 },
    { 70,	0,	"sd",		ENTRY_TYPE_DISC,	96,	4 },
    { 71,	0,	"sd",		ENTRY_TYPE_DISC,	112,	4 },
    { 88,	0,	"hd",		ENTRY_TYPE_DISC,	12,	6 },
    { 89,	0,	"hd",		ENTRY_TYPE_DISC,	14,	6 },
    { 90,	0,	"hd",		ENTRY_TYPE_DISC,	16,	6 },
    { 91,	0,	"hd",		ENTRY_TYPE_DISC,	18,	6 },
    { 94,	0,	"dasd",		ENTRY_TYPE_DISC,	0,	2 },
    { 0,	0,	NULL,		ENTRY_TYPE_ONE,		0,	0 },
  };

  struct stat s;
  struct entry *e;

  ssize_t ret = 0;

  unsigned int unit;
  unsigned int part;

  if (!strcmp("none", path))
    return snprintf(buf, n, "%s", path);

  if (stat(path,&s) == -1)
    return 0;

  e = entries;
  while (e->name != NULL) {
    if (major(s.st_rdev) == e->major && 
        ((e->type == ENTRY_TYPE_ONE && minor(s.st_rdev) == e->minor) ||
         (e->type != ENTRY_TYPE_ONE && minor(s.st_rdev) >= e->minor))) {
      break;
    }
    e++;
  }
  if ( ! e->name )
    /* Pass unknown devices on without changes.  This fixes LVM devices */
    return snprintf(buf, n, "%s", path);

  switch (e->type)
  {
    case ENTRY_TYPE_ONE:
      ret = snprintf(buf, n, "/dev/%s", e->name);
      break;

    case ENTRY_TYPE_NUMBER:
      unit = minor(s.st_rdev) - e->minor + e->entry_first;

      ret = snprintf(buf, n, "/dev/%s%d", e->name, unit);
      break;

    case ENTRY_TYPE_DISC:
      unit = (minor(s.st_rdev) >> e->entry_disc_minor_shift);
      part = (minor(s.st_rdev) & ((1 << e->entry_disc_minor_shift) -1 ));

      unit += e->entry_first;

      if (unit+'a' > 'z') {
        unit -= 26;
        ret = snprintf(buf, n, "/dev/%s%c%c", e->name, 'a' + unit / 26, 'a' + unit % 26);
      }
      else
        ret = snprintf(buf, n, "/dev/%s%c", e->name, 'a' + unit);

      if (part)
        ret = snprintf(buf + ret, n - ret, "%d", part);

      break;
  };

  return ret;
}

/*
 * Configure all dependencies, special case for virtual packages.
 * This is done depth-first.
 */
int di_config_package(struct package_t *p,
                      int (*virtfunc)(struct package_t *),
	              int (*walkfunc)(struct package_t *)) {
	char *configcommand;
	int ret, i;
	struct package_t *dep;

	if (virtfunc != NULL && di_pkg_is_virtual(p)) {
		return virtfunc(p);
	}

	for (i = 0; p->depends[i] != 0; i++) {
		if ((dep = p->depends[i]->ptr) == NULL)
			continue;
		if (dep->status == installed)
			continue;
		/* Recursively configure this package */
		if (!di_config_package(dep, virtfunc, walkfunc))
			return 0;
	}

	if (asprintf(&configcommand, "exec " DPKG_CONFIGURE_COMMAND " %s", p->package) == -1) {
		return 0;
	}
	ret = system(configcommand);
	free(configcommand);
        if (ret == 0) {
            p->status = installed;
	    if (walkfunc != NULL)
		    walkfunc(p);
        } else {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "Configuring '%s' failed with error code %d",
                     p->package, ret);
            di_log(buf);
            p->status = half_configured;
	    return 0;
	}
	return !ret;
}
