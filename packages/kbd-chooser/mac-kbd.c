/* @file  mackbd.c
 *
 * @brief Report keyboards present on Mac
 *
 * Copyright (C) 2003,2004 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id$
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
	k->data = NULL;
	k->next = keyboards;
	keyboards = k;

	// Handle 2.2 kernel.
	if (grep("/proc/version", "2.2.") > 0)
		k->present = TRUE;
	// if we send linux keycodes (grep > 0), don't use ADB keymaps
	// pretend we don't have an ADB keyboard
	// If keyboard_sends_linux_keycodes isn't present (grep < 0), then
	// ADB keymap support is not even compiled into the kernel.
	else if (grep("/proc/sys/dev/mac_hid/keyboard_sends_linux_keycodes", "1") != 0)
		k->present = FALSE;
	else
		k->present = TRUE;

#if 0
	if (k->present != UNKNOWN)
		return keyboards;
	
	// In 2.6 series, we can detect keyboard via /proc/bus/input
	// This bit of code should be superflous; 
	res = grep ("/proc/bus/input/devices", "ADB keyboard");
	if (res > 0)
		k->present = TRUE;
	else if (res == 0)
		k->present = FALSE;
	else
		k->present = UNKNOWN;
#endif
	// TODO:
	// We should be able to read the keyboard type from /proc/bus/input/devices too.
	//
	return keyboards;	
}
