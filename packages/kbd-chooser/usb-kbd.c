/* @file  usb-kbd.c
 * @brief Report keyboards present on PC
 *
 * Copyright (C) 2002,2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id$
 */

#include "config.h"
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "kbd-chooser.h"

/**
 * @brief list of keyboards present
 */
kbd_t *usb_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = xmalloc(sizeof(kbd_t));
	int serr, res, mounted_fs = 0 ;

	// Set up default entries.
	k->name = "usb";
	k->data = NULL;
	k->deflt = NULL;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;


#if !defined(__powerpc__)
	/* The following code tries to autodetect USB keyboards.
	 * its currently implicated in debian bug #234513,
	 * and excluded for beta3.
	 */
	
	/* In 2.4, if "Device=usbkbd" is present in /proc/bus/usb/devices
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
	
	res = grep ("/proc/bus/usb/devices", "Driver=usbkbd");
	if (res < 0) {
		if (DEBUG) 
			di_log (DI_LOG_LEVEL_DEBUG, "mounting usbdevfs to look for kbd");		
		// redirect stderr for the moment
		serr = dup(2);
		close (2);
		open ("/dev/null", O_RDWR);
		if (system ("mount -t  usbdevfs usbdevfs /proc/bus/usb") != 0) {		 
			di_warning ("Failed to mount USB filesystem to autodetect USB keyboard;");
			di_warning ("Will add USB keyboard option just in case");
			return keyboards;
		}
		// restore stderr
		close (2);
		dup (serr);
		mounted_fs = 1;
		res = grep ("/proc/bus/usb/devices", "Driver=usbkbd");
		if (res < 0) {
			di_warning ("Failed to grep /proc/bus/usb/drivers");
		}
	}	
	if (res == 0) 
		k->present = TRUE;
	if (res == 1)
		k->present = FALSE;
	if (mounted_fs)
		system ("umount /proc/bus/usb");

#endif

	return keyboards;	
}

/*
 * TODO:
 * USB keyboards should be able to figure out  a default
 * (read lang/country code from USB code) and set default.
 */
