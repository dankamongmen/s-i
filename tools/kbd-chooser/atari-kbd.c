/* @file  amiga-kbd.c
 *
 * @brief Report keyboards present on Amiga
 *
 * Copyright (C) 2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: atari-kbd.c,v 1.2 2003/10/12 19:56:23 mckinstry Exp $
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
kbd_t *atari_kbd_get (kbd_t *keyboards)
{
	kbd_t *k = xmalloc (sizeof (kbd_t));

	k->name = "atari"; // This must match the name "amiga" in console-keymaps-amiga
	k->description = _("Atari Keyboard");
	k->deflt = "atari-us";
	k->fd = -1;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;

		
#if defined (KERNEL_2_6)
	// FIXME Write this

#endif // KERNEL_2_6

	/* ***  Only reached if KERNEL_2_6 not present *** 
	 * For 2.4, assume a keyboard is present
	 */	
	return keyboards;
}
