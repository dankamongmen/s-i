/* @file  at-kbd.c
 * @brief Report keyboards present on PC
 *
 * Copyright (C) 2002 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: at-kbd.c,v 1.5 2003/03/02 12:58:05 mckinstry Exp $
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
void at_kbd_get ()
{
	kbd_t *k = xmalloc (sizeof(kbd_t));

	// /proc must be mounted by this point
	assert (di_check_dir ("/proc") == 1);

	k->name = "ps2"; // This must match the name "ps2" in console-keymaps-ps2
	k->description = N_("PC Style (PS2/connector)");
	k->deflt = NULL;
	k->fd = -1;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;


#if defined (KERNEL_2_5)
	/* In 2.5 series, we can detect keyboard via /proc/bus/input
	 *
	 */
	if (di_check_dir ("/proc/bus/input") >= 0) {
	        // this dir only present in 2.5
		res = grep ("/proc/bus/input/devices","AT Set ");
		if (res < 0) {
			di_log ("at-kbd: Failed to open /proc/bus/input/devices");
			return;
		}
		k->present = ( res == 0) ? TRUE : FALSE;
		return;
	}

#endif // KERNEL_2_5

	/* ***  Only reached if KERNEL_2_5 not present ***  */

	/* For 2.4, assume a PC keyboard is present
	 */
}
