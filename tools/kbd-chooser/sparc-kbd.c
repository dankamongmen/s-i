/* @file  sparc-kbd.c
 * @brief Report keyboards present on Sun
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: sparc-kbd.c,v 1.6 2003/10/03 22:02:49 mckinstry Exp $
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
kbd_t *at_kbd_get (kbd_t *keyboards)
{
	kbd_t *k = xmalloc (sizeof(kbd_t));

	// /proc must be mounted by this point
	assert (di_check_dir ("/proc") == 1);

	k->name = "sparc"; // This must match the name "sparc" in console-keymaps-sparc
	k->description = _("Sun Keyboard");
	k->deflt = "sunkeymap";
	k->fd = -1;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;
	
#if defined (KERNEL_2_6)
	/* In 2.6 series, we can detect keyboard via /proc/bus/input
	 *
	 * TODO:
	 * Its possible to read the keyboard type; use this to preselect
	 * keymap (edit the keymaps list)
	 */
#warning "Kernel 2.6 code not written yet"
	if (di_check_dir ("/proc/bus/input") >= 0) {
		int res;
		// this dir only present in 2.6
		res = grep ("/proc/bus/input/devices","Sun Type");
		if (res < 0) {
			di_warning ("sparc-kbd: Failed to open /proc/bus/input/devices");
			return keyboards;
		}
		k->present = (res == 0) ? TRUE : FALSE;
	}	


#endif // KERNEL_2_6

	/* ***  Only reached if KERNEL_2_6 not present ***  */

	/* For 2.4, assume a keyboard is present
	 */
	return keyboards;
}
