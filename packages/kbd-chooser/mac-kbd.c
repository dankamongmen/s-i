/* @file  mackbd.c
 *
 * @brief Report keyboards present on Mac
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: mac-kbd.c,v 1.8 2004/03/13 09:17:19 mckinstry Exp $
 */

#include "config.h"
#include <debian-installer.h>
#include "xmalloc.h"
#include "kbd-chooser.h"

/**
 * @brief list of keyboards present
 */
kbd_t *mac_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = NULL;

	if (strstr (subarch, "mac") == NULL)
		return keyboards;

	k = xmalloc (sizeof (kbd_t)); 
	k->name = "mac"; // This must match the name "mac" in console-keymaps-mac
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
	if (check_dir ("/proc/bus/input") >= 0) {
		// this dir only present in 2.6
	}


#endif // KERNEL_2_6

	/* ***  Only reached if KERNEL_2_6 not present ***  */

	/* For 2.4, assume a keyboard is present
	 */
	return keyboards;	
}
