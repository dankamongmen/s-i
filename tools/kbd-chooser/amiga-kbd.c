/* @file  amiga-kbd.c
 *
 * @brief Report keyboards present on Amiga
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: amiga-kbd.c,v 1.7 2003/10/14 20:42:02 mckinstry Exp $
 */

#include "config.h"
#include <assert.h>
#include <string.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "kbd-chooser.h"


/**
 * @brief list of keyboards present
 */
kbd_t *amiga_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = xmalloc (sizeof (kbd_t));

#if defined(__m68k__)
	if (strstr (subarch, "amiga")== NULL)
		return keyboards;
#endif
#if defined(__powerpc__)
	if  (strstr (subarch, "apus") == NULL)
		return keyboards;
#endif

	k->name = "amiga"; // This must match the name "amiga" in console-keymaps-amiga
	k->deflt = "amiga-us";
	k->fd = -1;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;

		
#if defined (KERNEL_2_6)

	// /proc must be mounted by this point
	assert (di_check_dir ("/proc") == 1);
	
	// In 2.6 series, we can detect keyboard via /proc/bus/input
	if (di_check_dir ("/proc/bus/input") >= 0) {
		int res;
		// this dir only present in 2.6
		res = grep ("/proc/bus/input/devices", "Amiga keyboard");
		if (res < 0) {
			di_warning ("amiga-kbd: Failed to open /proc/bus/input/devicves");
			return keyboards;
		}
		k->present = ( res == 0) ? TRUE : FALSE;
		return keyboards;
	}


#endif // KERNEL_2_6

	/* ***  Only reached if KERNEL_2_6 not present *** 
	 * For 2.4, assume a keyboard is present
	 */	
	return keyboards;
}
