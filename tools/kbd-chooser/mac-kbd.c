/* @file  mackbd.c
 *
 * @brief Report keyboards present on Mac
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: mac-kbd.c,v 1.5 2003/10/03 22:02:49 mckinstry Exp $
 */

#include "config.h"
#include <assert.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "nls.h"
#include "kbd-chooser.h"

/**
 * @brief list of keyboards present
 */
kbd_t *mac_kbd_get (kbd_t *keyboards)
{
	kbd_t *k = xmalloc (sizeof (kbd_t));

	// /proc must be mounted by this point
	assert (di_check_dir ("/proc") == 1);

	k->name = "mac"; // This must match the name "mac" in console-keymaps-mac
	k->description = _("Mac Keyboard");
	k->deflt = NULL;
	k->fd = -1;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;
	
#if defined (KERNEL_2_6)
	/* In 2.6 series, we can detect keyboard via /proc/bus/input
	 *
	 * FIXME: Write this code
	 */
#warning "Kernel 2.6 code not written yet"
	if (di_check_dir ("/proc/bus/input") >= 0) {
		// this dir only present in 2.6
	}


#endif // KERNEL_2_6

	/* ***  Only reached if KERNEL_2_6 not present ***  */

	/* For 2.4, assume a keyboard is present
	 */
	return keyboards;	
}
