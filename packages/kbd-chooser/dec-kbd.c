/* @file  dec-kbd.c
 * @brief Report keyboards present on DEC
 *
 * Copyright (C) 2004 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: dec-kbd.c 11900 2004-03-24 21:52:04Z mckinstry $
 */

#include "config.h"
#include <assert.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "kbd-chooser.h"


/**
 * @brief list of keyboards present
 */
kbd_t *dec_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = xmalloc (sizeof(kbd_t));

	k->name = "dec"; // This must match the name "dec" in console-keymaps-sun
	k->deflt = "lk201-us" ; // At the moment, this is the only DEC keyboard
	k->data = NULL;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;
	
	return keyboards;
}
