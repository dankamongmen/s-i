/* 

   choosetable.c - Part of autopartkit, a module to partition devices
                   for debian-installer.

   Author - Petter Reinholdtsen

   Copyright (C) 2002  Petter Reinholdtsen <pere@hungry.com>
   
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

#include <string.h>
#include <stdlib.h>

#include "autopartkit.h"

#if defined(TEST)
#define DATADIR "."
#else /* not TEST */
#define DATADIR "/etc/autopartkit"
#endif /* not TEST */
 
#define SERVER      0x1
#define WORKSTATION 0x2
#define LTSP_SERVER 0x4

static int add_profile(int curmask, const char *profile)
{
    /* Skip initial space */
    if (' ' == *profile)
        profile++;

    if (strncmp(profile, "Server", 6) == 0)
    {
	autopartkit_log(2, "Adding server flag\n");
        curmask |= SERVER;
    }
    else if (strncmp(profile, "Workstation", 11) == 0)
    {
	autopartkit_log(2, "Adding workstation flag\n");
        curmask |= WORKSTATION;
    }
    else if (strncmp(profile, "LTSP-server", 11) == 0)
    {
	autopartkit_log(2, "Adding tlsp-server flag\n");
        curmask |= LTSP_SERVER;
    }
    else
	autopartkit_log(0, "Unknown profile '%s', not setting any flags\n",
			profile);

    return curmask;
}

/*
 * Handle the fact that the debian-installer/profile value is
 * multiselect.
 *
 * Mapping:
 *   Server                             => Server
 *   Server + Workstation               => Server+Workstation
 *   Server + LTSP-server               => Server+LTSP-server
 *   Server + Workstation + LTSP-server => Server+LTSP-server
 *   Workstation                        => Workstation
 *   LTSP-server                        => Workstation+LTSP-server
 *   Workstation + LTSP-server          => Workstation+LTSP-server
 */
const char *
choose_profile_table(const char *profiles)
{
    /* Make a copy of profiles to avoid modifying the original in strsep() */
    char *valuestr;
    char *str;
    int profilemask = 0;
    char *tablefile;

    /* Avoid segfault, and make sure default.table is choosen. */
    if (NULL == profiles)
      profiles = "";

    valuestr = strdup(profiles);
    /* assert(valuestr); */

    autopartkit_log(2, "Profile is '%s'\n", valuestr);

    if (strchr(valuestr, ','))
        for (; (str = strsep(&valuestr, ",")); )
	{
	    profilemask = add_profile(profilemask, str);
	}
    else
        profilemask = add_profile(profilemask, valuestr);

    free(valuestr);
    valuestr = NULL;

    switch (profilemask) {
    case SERVER:
        tablefile = DATADIR "/Server.table";
	break;
    case SERVER | WORKSTATION:
        tablefile = DATADIR "/Server+Workstation.table";
	break;
    case SERVER | LTSP_SERVER:
    case SERVER | LTSP_SERVER | WORKSTATION:
        tablefile = DATADIR "/Server+LTSP-server.table";
	break;
    case LTSP_SERVER:
    case WORKSTATION | LTSP_SERVER:
        tablefile = DATADIR "/Workstation+LTSP-server.table";
	break;
    case WORKSTATION:
        tablefile = DATADIR "/Workstation.table";
	break;
    default:
        tablefile = DATADIR "/default.table";
	break;
    }	  
    return tablefile;
}
