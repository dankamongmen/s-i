/* @file  amiga-kbd.c
 *
 * @brief Report keyboards present on Amiga
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: amiga-kbd.c,v 1.2 2003/01/28 11:02:36 mckinstry Exp $
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
