/* @file  sparc-kbd.c
 * @brief Report keyboards present on Sun
 *
 * Copyright (C) 2003,2004 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id$
 */

#include "config.h"
#include <assert.h>
#include <debian-installer.h>
#include <sys/utsname.h>
#include "xmalloc.h"
#include "kbd-chooser.h"

/**
 * @brief list of keyboards present
 */
kbd_t *sparc_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = NULL;
	struct utsname buf;

	uname(&buf);

#if defined(__m68k__)
	// on m68k only sun3(x) have PC-style keyboards
	if (strstr(subarch, "sun3") == NULL) 
		return keyboards;
#endif
    
	k = xmalloc (sizeof(kbd_t));

	/* 2.6 enumerates AT keymap */
	if (strncmp(buf.release, "2.6", 3) == 0)
		k->name = "at";
	else
		k->name = "sun"; // This must match the name "sun" in console-keymaps-sun
	k->deflt = NULL;
	k->data = NULL;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;
	
	/* In 2.6 series, we can detect keyboard via /proc/bus/input
	 *
	 * TODO:
	 * Its possible to read the keyboard type; use this to preselect
	 * keymap (edit the keymaps list)
	 */

	  // /proc must be mounted by this point
	  // assert (check_dir ("/proc") == 1); 
	
	  if (check_dir ("/proc/bus/input")) {
		int res;
		// this dir only present in 2.6
		res = grep ("/proc/bus/input/devices", "Sun Type");
		if (res < 0) {
			di_info ("sparc-kbd: Could not open /proc/bus/input/devices");
			return keyboards;
		}
		k->present = (res > 0) ? TRUE : FALSE;
	}	

	return keyboards;
}
