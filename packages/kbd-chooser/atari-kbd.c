/* @file  atari-kbd.c
 *
 * @brief Report keyboards present on Atari
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id$
 */

#include "config.h"
#include <string.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "kbd-chooser.h"


/**
 * @brief list of keyboards present
 */
kbd_t *atari_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = xmalloc (sizeof (kbd_t));

	if (strstr (subarch, "atari") == NULL)
		return keyboards;

	k->name = "atari"; // This must match the name "atari" in console-keymaps-atari
	k->deflt = NULL;
	k->data = NULL;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;

		
#if defined (KERNEL_2_6)
	// In 2.6 series, we can detect keyboard via /proc/bus/input
	// FIXME Write this

#endif // KERNEL_2_6

	/* ***  Only reached if KERNEL_2_6 not present *** 
	 * For 2.4, assume a keyboard is present
	 */	
	return keyboards;
}
