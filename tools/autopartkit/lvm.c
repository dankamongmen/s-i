/*
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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

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
    /* Is /proc/lvm a directory? */
    if ( 0 != stat("/proc/lvm", &statbuf)
	 || ! S_ISDIR(statbuf.st_mode) )
    {
        autopartkit_error(0, "Missing /proc/lvm/, no LVM support available.");
	return FALSE;
    }
  
    /* Is /bin/vgscan available? */
    if (0 != stat(VGSCAN, &statbuf) || ! S_ISREG(statbuf.st_mode))
    {
        autopartkit_error(0, "Missing %s, no LVM support available.", VGSCAN);
        return FALSE;
    }

    return TRUE;
}

/*
 * Return true = 1 if the volume group exist, and false = 0 if it doesn't.
 */
static int
vg_exists(const char *vgname)
{
    struct stat statbuf;
    char *cmd = NULL;

    if (NULL == vgname)
        return FALSE;

    asprintf(&cmd, "/proc/lvm/VGs/%s", vgname);
    if ( ! cmd ||
	 0 != stat(cmd, &statbuf)
	 || ! S_ISDIR(statbuf.st_mode) )
    {
        autopartkit_error(0, "Missing volume group '%s'", cmd);
	if (cmd)
	    free(cmd);
	return FALSE;
    }
    return FALSE;
}

int
lvm_init(void)
{
    int retval;
    if ( ! lvm_isinstalled())
        return -1;
    /* Call vgscan */
    retval = system("vgscan > /var/log/messages 2>&1");
    return 0;
}

int
lvm_init_dev(const char *devpath)
{
    char * cmd = NULL;
    int retval = 0;

    if ( ! lvm_isinstalled())
        return -1;

    asprintf(&cmd, "pvcreate %s > /var/log/messages 2>&1", devpath);
    retval = system(cmd);
    if (cmd)
        free(cmd);
    return retval;
}

/*
 * Create new volumegroup if it do not exist, and add new physical
 * device (devpath) to the voluemgroup if it exists.
 */
int
lvm_volumegroup_add_dev(const char *vgname, const char *devpath)
{
    int retval = 0;
    char *progname = NULL;
    char * cmd = NULL;

    if ( ! lvm_isinstalled())
        return -1;

    if (vg_exists(vgname))
        progname = "vgextend";
    else
        progname = "vgcreate";

    asprintf(&cmd, "%s %s %s > /var/log/messages 2>&1", progname,
	     vgname, devpath);
    retval = system(cmd);

    if (cmd)
        free(cmd);
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
    char * str = NULL;
    int retval;


    if ( ! lvm_isinstalled())
        return NULL;

    asprintf(&str, "lvcreate -n%s -L%d %s > /var/log/messages 2>&1",
	     lvname, mbsize, vgname);

    retval = system(str);

    if (str)
      free(str);
    str=NULL;

    if (0 == retval)
    {
	asprintf(&str, "/dev/%s/%s", vgname, lvname);
	return str;
    }
    else 
        return NULL;
}
