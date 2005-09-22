/*

   lvm.c - Part of autopartkit, a module to partition devices
                  for debian-installer.

   Author - Petter Reinholdtsen

   Copyright (C) 2003  Petter Reinholdtsen <pere@hungry.com>
   
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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#define _GNU_SOURCE /* for asprintf() in <stdio.h> */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "autopartkit.h"

typedef enum {
    FALSE = 0,
    TRUE = 1
} BOOLEAN;

static const char *VGSCAN = "/sbin/vgscan";

static BOOLEAN
lvm_isinstalled(void)
{
    struct stat statbuf;
    static int isinstalled = -1;

    if (isinstalled != -1)
      return isinstalled;

    if (0 != stat("/etc/uselvm", &statbuf) || ! S_ISREG(statbuf.st_mode))
    {
        autopartkit_log(1, "Missing /etc/uselvm, no LVM support enabled.");
        isinstalled = FALSE;
        return FALSE;
    }

    /* Is /proc/lvm a directory, or device-mapper loaded ? */
    if ((0 != system("grep -q \"[0-9] device-mapper\" /proc/misc" )) &&
        (0 != stat("/proc/lvm", &statbuf) || ! S_ISDIR(statbuf.st_mode)))
    {
        autopartkit_error(0, "Missing /proc/lvm/ and no device-mapper in /proc/nmisc, no LVM support available.");
        isinstalled = FALSE;
        return FALSE;
    }
  
    /* Is /bin/vgscan available? */
    if (0 != stat(VGSCAN, &statbuf) || ! S_ISREG(statbuf.st_mode))
    {
        autopartkit_error(0, "Missing %s, no LVM support available.", VGSCAN);
        isinstalled = FALSE;
        return FALSE;
    }

    isinstalled = TRUE;
    return TRUE;
}

/*
 * Return true = 1 if the volume group exist, and false = 0 if it doesn't.
 */
static int
vg_exists(const char *vgname)
{
    struct stat statbuf;
    char *devpath = NULL;
    int retval = FALSE;
    char *command = NULL;

    if (NULL == vgname)
        return FALSE;

    /* If the VG is missing, text is only printed on stderr.  Throw
       away this, to detect if the VG existed or not. */
    asprintf(&command, "vgdisplay \"%s\" 2>/dev/null |grep -q \"%s\"",
	     vgname, vgname);

    if ( NULL == command ) {
        autopartkit_error(0, "Unable to allocate string for command");
	return retval;
    }
    if (0 == system(command))
	retval = TRUE;
    free(command);
    return retval;
}

int
lvm_init(void)
{
    int retval = -1;
    if ( ! lvm_isinstalled())
        return retval;

    if (0 == (retval = system("log-output -t autopartkit vgscan")))
	retval = 0;
    else
        autopartkit_log(2, "Executing vgscan returned error code %d\n",retval);

    return retval;
}

int
lvm_init_dev(const char *devpath)
{
    char * cmd = NULL;
    int retval = -1;

    if ( ! lvm_isinstalled())
        return -1;

    autopartkit_log(1, "Initializing LVM pv '%s'\n",
                    devpath ? devpath : "(null)");

    asprintf(&cmd, "log-output -t autopartkit pvcreate %s", devpath);
    if (cmd)
    {
        retval = system(cmd);
        free(cmd);
    }
    return retval;
}

/*
 * Create new volumegroup if it do not exist, and add new physical
 * device (devpath) to the voluemgroup if it exists.
 */
int
lvm_volumegroup_add_dev(const char *vgname, const char *devpath)
{
    int retval;
    char *progname = NULL;
    char * cmd = NULL;

    if ( ! lvm_isinstalled())
        return -1;

    autopartkit_log(1, "Adding LVM pv '%s' to vg '%s'\n",
                    devpath ? devpath : "(null)",
                    vgname ? vgname : "(null)");

    /* Call vgscan first, it seem to be required for vgcreate to work. */
    if (0 != (retval = system("log-output -t autopartkit vgscan")))
        autopartkit_log(2, "Executing vgscan returned error code %d\n",retval);

    if (vg_exists(vgname))
        progname = "vgextend";
    else
        progname = "vgcreate";

    asprintf(&cmd, "log-output -t autopartkit %s %s %s", progname,
             vgname, devpath);

    retval = -1;
    if (cmd)
    {
	retval = system(cmd);
        free(cmd);
    }
    return retval;
}

/*
 * Create Logical Volume in given volume group, and return device path
 * if successfull.  Return NULL on failure.  The string is malloc()ed,
 * and must be free()d by the caller.
 */
char *
lvm_create_logicalvolume(const char *vgname, const char *lvname,
                       unsigned int mbsize)
{
    char *cmd = NULL;
    char *devpath = NULL;
    int retval = -1;

    if ( ! lvm_isinstalled())
        return NULL;

    autopartkit_log(1, "Creating LVM lv '%s' on vg '%s' (size=%d MiB)\n",
                    lvname ? lvname : "(null)",
                    vgname ? vgname : "(null)",
                    mbsize);

    asprintf(&cmd, "log-output -t autopartkit lvcreate -n%s -L%d %s",
             lvname, mbsize, vgname);
    if (cmd)
    {
	retval = system(cmd);
	free(cmd);
	cmd=NULL;
    }

    if (0 == retval)
    {
        asprintf(&devpath, "/dev/%s/%s", vgname, lvname);
        return devpath;
    }
    else 
        return NULL;
}

/* Extract vgname, lvname and fstype from "lvm:tjener_vg:home0_lv:default". */
int
lvm_split_fstype(const char *str, int separator, int elemcount,
		 char *elements[])
{
    int elemnum;
    const char *curp = str;
    const char *nextp;

    for (elemnum = 0 ; elemnum < elemcount; elemnum++)
    {
	nextp = strchr(curp, separator);
	if (NULL == nextp)  /* Last element */
	  {
	    elements[elemnum] = strdup(curp);
	    autopartkit_log(3, "Found last '%s'\n", elements[elemnum]);
	    return 0;
	  }
	elements[elemnum] = strndup(curp, nextp - curp);
	autopartkit_log(3, "Found '%s'\n", elements[elemnum]);
	curp = nextp + 1;
    }
    
    return 0;
}

char *
lvm_lv_add(void *stack, const char *fstype, unsigned int mbminsize,
	   unsigned int mbmaxsize)
{
    /* Create LVM logical volume with given FS.  Assuming LVM volume
       group is already created. */
    char *info[4]; /* 0='lvm', 1=vgname, 2=lvname, 3=fstype */

    /* Extract vgname, lvname and fstype from
       "lvm:tjener_vg:home0_lv:default". */
    if (0 != lvm_split_fstype(fstype, ':', 4, info))
    {
        autopartkit_log(0, "  Failed to parse '%s'\n", fstype);
	return NULL;
    }
    if (0 == strcmp("default", info[3]))
    {
        char *s = strdup(DEFAULT_FS);
	if (!s)
	    return NULL;
	else
	{
	    free(info[3]);
	    info[3] = s;
	}
    }
    autopartkit_log(2, "  Stacking LVM lv %s on vg %s "
		    "fstype %s\n", info[1], info[2], info[3]);
    /* Store vgname, lvname and size in stack */
    if (0 == lvm_lv_stack_push(stack, info[1], info[2], info[3],
			       mbminsize, mbmaxsize))
    {
        char buf[1024];
	snprintf(buf, sizeof(buf), "%s:%s", info[1], info[2]);
	return strdup(buf);
	
    }
    return NULL;
}

/* (andread@linpro.no) */
int
lvm_get_free_space_list(char *vgname, struct disk_info_t *spaceinfo)
{
    FILE *vgdisplay = NULL;
    char buf[160];
    char *command = NULL;

    spaceinfo->path = strdup(vgname);
    spaceinfo->capacity = (PedSector)0;
    spaceinfo->freespace = (PedSector)0;
    memset(&(spaceinfo->geom), 0, sizeof(spaceinfo->geom));
    autopartkit_log(2, "  Locating free space on volumegroup %s\n", vgname);

    /*
     * Redirect errors to /dev/null, to avoid lots of messages claiming
     * "File descriptor [3,4,5,6,7] left open", making it hard to find the
     * VG size.
     */
    asprintf(&command, "/sbin/vgdisplay -c %s 2>/dev/null", vgname);
    autopartkit_log(2, "  Running command: %s\n", command);
    vgdisplay = popen(command, "r");
    if (! vgdisplay ){
        autopartkit_log(0, "Failed to run vgdisplay\n");
	free(spaceinfo);
	free(command);
	return 0;
    }
    if (fgets(buf, 160, vgdisplay) != NULL){
	char *token = NULL;
	int i = 0;
	int pe_size = 0;
	int free_pe = 0;
	int vg_size = 0;

        autopartkit_log(3, "  Vgdisplay: %s", buf);
	token = strtok(buf, ":");
	if (token) {
	  while (token){
	    i++;
	    /*
	      We expect the tokenized output fields to be like this:
	      12 - size of volume group in kilobytes
	      13 - physical extent size
	      16 - free number of physical extents for this volume group
	    */
	    /* printf("DEBUG (%i): %s\n", i, token); */
	    if (i == 12) vg_size = atoi(token);
	    else if (i == 13) pe_size = atoi(token);
	    else if (i == 16)free_pe = atoi(token);
	    token = strtok(NULL, ":");
	  }
	} 
	else {
	  /* No tokens */
	  free(spaceinfo);
	  free(command);
	  return 0;
	}
	/* FIXME: Some problems with the data from vgdisplay -c. Using
           field 12 for now instead */
	/*
	spaceinfo->capacity = 
	  (PedSector)(MiB_TO_BLOCKS(pe_size * free_pe / (1024) ));
	spaceinfo->freespace = 
	  (PedSector)(MiB_TO_BLOCKS(pe_size * free_pe / (1024) ));
	*/
	spaceinfo->capacity = 
	  (PedSector)(MiB_TO_BLOCKS(vg_size / (1024) ));
	spaceinfo->freespace = 
	  (PedSector)(MiB_TO_BLOCKS(vg_size / (1024) ));
    }
    else {
        autopartkit_log(0, "Failed to find vg size\n");
        free(spaceinfo);
	free(command);
        return 0;
    }
    pclose (vgdisplay);
    free(command);
    return 1;
}

/* LVM stack operations */

/* vg-stack (andread@linpro.no) */

struct lvm_vg_info {
    struct lvm_vg_info *next;
    char *vgname;
};

void *
lvm_vg_stack_new()
{
    struct lvm_vg_info *head;
    head = malloc(sizeof(*head));
    if (NULL != head)
      {
          head->next = head;
	  head->vgname = NULL;
      }
    return head;
}

int
lvm_vg_stack_isempty(void *stack)
{
    struct lvm_vg_info *head = stack;
    return head->next == head;
}

int
lvm_vg_stack_push(void *stack, const char *vgname)
{
    struct lvm_vg_info *head = stack;
    struct lvm_vg_info *elem;

    elem = malloc(sizeof(*elem));
    if (NULL == elem)
        return -1;
    elem->vgname = strdup(vgname);

    elem->next = head->next;
    head->next = elem;

    return 0;
}

int
lvm_vg_stack_pop(void *stack, char **vgname)
{
    struct lvm_vg_info *head = stack;
    struct lvm_vg_info *elem;

    elem = head->next;

    if (elem == head)
        return -1;

    head->next = elem->next;

    *vgname = elem->vgname;

    free(elem);
  
    return 0;
}

int
lvm_vg_stack_delete(void *stack)
{
    char *vgname;
    while ( ! lvm_vg_stack_isempty(stack) )
    {
	lvm_vg_stack_pop(stack, &vgname);
	free(vgname);
    }
    free(stack);
    return 0;
}

/* --- */

struct lvm_pv_info {
    struct lvm_pv_info *next;
    char *vgname;
    char *devpath;
};

void *
lvm_pv_stack_new()
{
    struct lvm_pv_info *head;
    head = malloc(sizeof(*head));
    if (NULL != head)
    {
        head->next = head;
	head->vgname = NULL;
	head->devpath = NULL;
    }
    return head;
}
int
lvm_pv_stack_isempty(void *stack)
{
    struct lvm_pv_info *head = stack;
    return head->next == head;
}
int
lvm_pv_stack_push(void *stack, const char *vgname, const char *devpath)
{
    struct lvm_pv_info *head = stack;
    struct lvm_pv_info *elem;

    elem = malloc(sizeof(*elem));
    if (NULL == elem)
        return -1;
    elem->vgname = strdup(vgname);
    elem->devpath = strdup(devpath);

    elem->next = head->next;
    head->next = elem;

    return 0;
}
int
lvm_pv_stack_pop(void *stack, char **vgname, char **devpath)
{
    struct lvm_pv_info *head = stack;
    struct lvm_pv_info *elem;

    elem = head->next;

    if (elem == head)
        return -1;

    head->next = elem->next;

    *vgname = elem->vgname;
    *devpath = elem->devpath;

    free(elem);
  
    return 0;
}
int
lvm_pv_stack_delete(void *stack)
{
    char *vgname;
    char *devpath;
    while ( ! lvm_pv_stack_isempty(stack) )
    {
	lvm_pv_stack_pop(stack, &vgname, &devpath);
	free(vgname);
	free(devpath);
    }
    free(stack);
    return 0;
}

struct lvm_lv_info { /* Store vgname, lvname and mbsize in list */
    struct lvm_lv_info *next;
    char *vgname;
    char *lvname;
    char *fstype;
    unsigned int mbminsize;
    unsigned int mbmaxsize;
};

void *
lvm_lv_stack_new()
{
    struct lvm_lv_info *head;
    head = malloc(sizeof(*head));
    if (NULL != head)
    {
	head->next = head;
	head->vgname = NULL;
	head->lvname = NULL;
	head->mbminsize = 0;
	head->mbmaxsize = 0;
    }
    return head;
}
int
lvm_lv_stack_isempty(void *stack)
{
    struct lvm_lv_info *head = stack;
    return head->next == head;
}
int
lvm_lv_stack_push(void *stack, const char *vgname, const char *lvname,
		  const char *fstype, unsigned int mbminsize,
		  unsigned int mbmaxsize)
{
    struct lvm_lv_info *head = stack;
    struct lvm_lv_info *elem;

    elem = malloc(sizeof(*elem));
    if (NULL == elem)
        return -1;
    elem->vgname = strdup(vgname);
    elem->lvname = strdup(lvname);
    elem->fstype = strdup(fstype);
    elem->mbminsize = mbminsize;
    elem->mbmaxsize = mbmaxsize;

    elem->next = head->next;
    head->next = elem;

    return 0;
}
int
lvm_lv_stack_pop(void *stack, char **vgname, char **lvname, char **fstype,
		 unsigned int *mbminsize, unsigned int *mbmaxsize)
{
    struct lvm_lv_info *head = stack;
    struct lvm_lv_info *elem;

    elem = head->next;

    if (elem == head)
        return -1;

    head->next = elem->next;

    *vgname = elem->vgname;
    *lvname = elem->lvname;
    *fstype = elem->fstype;
    *mbminsize = elem->mbminsize;
    *mbmaxsize = elem->mbmaxsize;

    free(elem);
  
    return 0;
}
int
lvm_lv_stack_delete(void *stack)
{
    char *vgname;
    char *lvname;
    while ( ! lvm_lv_stack_isempty(stack) )
    {
	lvm_pv_stack_pop(stack, &vgname, &lvname);
	free(vgname);
	free(lvname);
    }
    free(stack);
    return 0;
}
