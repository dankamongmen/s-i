/* @file  amiga-kbd.c
 *
 * @brief Report keyboards present on Amiga
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: amiga-kbd.c,v 1.3 2003/01/29 09:52:15 mckinstry Exp $
 */

#include "config.h"
#include <assert.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "nls.h"
#include "kbd-chooser.h"

extern kbd_t *keyboards;

/**
 * @brief list of keyboards present
 */
void amiga_kbd_get ()
{
	kbd_t *k = xmalloc (sizeof (kbd_t));
	int res;

	// /proc must be mounted by this point
	assert (di_check_dir ("/proc") == 1);

	k->name = "amiga"; // This must match the name "amiga" in console-keymaps-amiga
	k->description = N_("Amiga Keyboard");
	k->deflt = NULL;
	k->fd = -1;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;

		
#if defined (KERNEL_2_5)
	// In 2.5 series, we can detect keyboard via /proc/bus/input
	if (di_check_dir ("/proc/bus/input") >= 0) {
		int res;
		// this dir only present in 2.5
		res = grep ("/proc/bus/input/devices", "Amiga keyboard");
		if (res < 0) {
			di_log ("amiga-kbd: Failed to open /proc/bus/input/devicves");
			return;
		}
		k->present = ( res == 0) ? TRUE : FALSE;
		return;
	}


#endif // KERNEL_2_5

	/* ***  Only reached if KERNEL_2_5 not present ***  */

	/* For 2.4, assume a keyboard is present
	 */	
}
