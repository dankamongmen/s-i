/* @file  at-kbd.c
 * @brief Report keyboards present on PC
 *
 * Copyright (C) 2002,2004 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id$
 */

#include "config.h"
#include <assert.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "kbd-chooser.h"


/**
 * @brief list of keyboards present
 */
kbd_t *at_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = NULL;

#if defined(__m68k__)
	// on m68k only mvme, bvme, and q40 have PC-style keyboards
	if ((strstr(subarch, "vme") == NULL) &&
	    (strstr(subarch, "q40") == NULL))
		return keyboards;
#endif
#if defined(__powerpc__)
	// On powerpc, prep and chrp machines have pc keyboards
	if ((strstr(subarch, "prep") == NULL) &&
	    (strstr(subarch, "chrp") == NULL))
		return keyboards;
#endif
#if defined(__mipsel__)
	// DECstations do not have AT keyboards
	if (strstr(subarch, "r3k-kn02") || strstr(subarch,"r4k-kn04"))
		return keyboards;
#endif

	k =  xmalloc (sizeof(kbd_t));	
	k->name = "at"; // This must match the name "at" in console-keymaps-at
	k->deflt = NULL;
	k->data = NULL;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;


	// /proc must be mounted by this point
	// assert (check_dir ("/proc") == 1);

	/* In 2.6 series, we can detect keyboard via /proc/bus/input
	 *
	 */
	if (check_dir ("/proc/bus/input")) {
		if ((grep ("/proc/bus/input/devices","AT Set ") == 0) ||
		    (grep ("/proc/bus/input/devices","AT Translated Set") == 0) ||
		    (grep ("/proc/bus/input/devices","AT Raw Set") ==0))
			k->present = TRUE;
		else
			k->present = FALSE;
	}

	return keyboards;
}
