/* @file  sparc-kbd.c
 * @brief Report keyboards present on Sun
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: sparc-kbd.c,v 1.10 2004/02/08 01:04:38 jbailey Exp $
 */

#include "config.h"
#include <assert.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "kbd-chooser.h"


/**
 * @brief list of keyboards present
 */
kbd_t *sparc_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = xmalloc (sizeof(kbd_t));

	k->name = "sun"; // This must match the name "sun" in console-keymaps-sun
	k->deflt = NULL;
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

	  // /proc must be mounted by this point
	  assert (check_dir ("/proc") == 1); 
	
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
