/* @file  acorn-kbd.c
 *
 * @brief Report keyboards present on Acorn
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: acorn-kbd.c,v 1.1 2003/01/28 11:00:39 mckinstry Exp $
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
void acorn_kbd_get ()
{
	kbd_t *k = xmalloc (sizeof (kbd_t));
	int res;

	// /proc must be mounted by this point
	assert (di_check_dir ("/proc") == 1);

	k->name = "acorn"; // This must match the name "acorn" in console-keymaps-acorn
	k->description = N_("Acorn Keyboard");
	k->deflt = NULL;
	k->fd = -1;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;

	res = grep ("/proc/cpuinfo", "Acorn");
	if (res < 0) {
		di_log ("amiga-kbd: failed to open /proc/cpuinfo.");
		return;
	}
	if (res != 0) { // Not an acorn
		k->present = FALSE;
		return;
	}
		
#if defined (KERNEL_2_5)
	/* In 2.5 series, we can detect keyboard via /proc/bus/input
	 *
	 * FIXME: Write this code
	 */
#warning "Kernel 2.5 code not written yet"
	if (di_check_dir ("/proc/bus/input") >= 0) {
		// this dir only present in 2.5
	}


#endif // KERNEL_2_5

	/* ***  Only reached if KERNEL_2_5 not present ***  */

	/* For 2.4, assume a keyboard is present
	 */	
}
