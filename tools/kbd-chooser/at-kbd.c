/* @file  at-kbd.c
 * @brief Report keyboards present on PC
 *
 * Copyright (C) 2002 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: at-kbd.c,v 1.1 2003/01/19 11:37:56 mckinstry Exp $
 */

#include "config.h"
#include <cdebconf/common.h>  // for NEW()

#include "kbd-chooser.h"

extern kbd_t *keyboards;

/**
 * @brief list of keyboards present
 */
kbd_t *at_kbd_get ()
{
	kbd_t *k = NEW (kbd_t);
	
#if defined (KERNEL_2_5)
	/* In 2.5 series, we can detect keyboard via /proc/bus/input
	 *
	 * FIXME: Write this code
	 */
#warning "Kernel 2.5 code not written yet"
	free (k);
	k = NULL;	
#else
	/* For 2.4, assume a PC keyboard is present
	 */
	k->name = "ps2"; // This must match the name "ps2" in console-keymaps-ps2
	k->fd = -1;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;
#endif
	
	return k;
}
