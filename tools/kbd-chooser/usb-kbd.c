/* @file  usb-kbd.c
 * @brief Report keyboards present on PC
 *
 * Copyright (C) 2002 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: usb-kbd.c,v 1.5 2003/01/28 11:02:36 mckinstry Exp $
 */

#include "config.h"
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "nls.h"
#include "kbd-chooser.h"

extern kbd_t *keyboards;	/* in kbd-chooser.c */


/**
 * @brief list of keyboards present
 */
void usb_kbd_get (void)
{
	kbd_t *k = xmalloc(sizeof(kbd_t));
	int res, mounted_fs = 0 ;

	// Set up default entries.
	k->name = "usb";
	k->description = N_("USB");
	k->fd = -1;
	k->deflt = NULL;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;
	
#if defined (KERNEL_2_5)
	/* In 2.5 series, we can detect keyboard via /proc/bus/input
	 *
	 * FIXME: Write this code	 
	 */	
#warning "Kernel 2.5 code not written yet"

#else /* 2.4 code */

	/* In 2.4, if "keyboard" is present in /proc/bus/usb/drivers,
	 * a USB keyboard is present.
	 * There may be multiple keyboards, but they can only have one
	 * map, so one choice to be made.
	 * If /proc/bus/usb/drivers does not exist,
	 *  "mount -t usbdevfs usbdevfs /proc/bus/usb"
	 *
	 * PROBLEMS:
	 *   (1) kernel module means there _probably_was_ a USB keyboard,
	 *     not that there is one at the moment.
	 */
	
	res = grep ("/proc/bus/usb/drivers", "keyboard");
	if (res < 0) {
		if (DEBUG) 
			di_log ("Failed to grep /proc/bus/usb/drivers; assuming usbdevfs not present");		
		res = system ("mount -t  usbdevfs usbdevfs /proc/bus/usb");
		if (res != 0) {
			di_log ("Failed to mount USB filesystem");
			return;
		}
		mounted_fs = 1;
		res = grep ("/proc/bus/usb/drivers", "keyboard");
		if (DEBUG && res < 0) 
			perror ("Failed to grep /proc/bus/usb/drivers");
	}	
	if (res >  0) {
		if (DEBUG) di_log ("Found USB Keyboard\n");
		k->present = TRUE;		
	}
	if (res == 0) {
		if (DEBUG) di_log ("No USB keyboard present\n");
		k->present = FALSE;
	}
	if (mounted_fs)
		system ("umount /proc/bus/usb");
	
#endif	
}

/*
 * TODO:
 * USB keyboards should be able to figure out  a default
 * (read lang/country code from USB code) and set default.
 */
