/* @file  hil-kbd.c
 * @brief Report keyboards present on DEC
 *
 * Copyright (C) 2004 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: hil-kbd.c 11900 2004-03-24 21:52:04Z mckinstry $
 */

#include "config.h"
#include <assert.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "kbd-chooser.h"

// HIL keyboards have a code (can be read from kernel); we should use
// this to pick keymap

// 0x00 Reserved 
// 0x01 Reserved 
// 0x02 Reserved
// 0x03 Swiss/French
// 0x04 Portuguese 
// 0x05 Arabic
// 0x06 Hebrew 
// 0x07 Canadian English
// 0x08 Turkish
// 0x09 Greek 
// 0x0a Thai (Thailand)
// 0x0b Italian
// 0x0c Hangul (Korea)
// 0x0d Dutch
// 0x0e Swedish
// 0x0f German
// 0x10 Chinese-PRC
// 0x11 Chinese-ROC 
// 0x12 Swiss/French II 
// 0x13 Spanish 
// 0x14 Swiss/German II
// 0x15 Belgian (Flemish)
// 0x16 Finnish 
// 0x17 United Kingdom
// 0x18 French/Canadian
// 0x19 Swiss/German
// 0x1a Norwegian 
// 0x1b French
// 0x1c Danish 
// 0x1d Katakana
// 0x1e Latin American/Spanish
// 0x1f United States

	
// Arch-specific information about HIL keyboards.
typedef struct { 
	uint16_t type;
	const char *default_kbd;
} hil_data;

/**
 * @brief list of keyboards present
 */
kbd_t *hil_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = xmalloc (sizeof(kbd_t));

	k->name = "hil"; // This must match the name "hil" in console-keymaps-sun
	k->deflt = NULL;
	k->data = NULL;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;

	// TODO : Add code here to detect keyboard type.
	
	if (check_dir("/proc/bus/input")) {
		int res;
		res = grep ("/proc/bus/input/devices", "HIL keyboard");
		if (res > 0)
			k->present = TRUE;
		else
			k->present = FALSE;
	}
	return keyboards;
}
