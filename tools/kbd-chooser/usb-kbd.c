/* @file  usb-kbd.c
 * @brief Report keyboards present on PC
 *
 * Copyright (C) 2002 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: usb-kbd.c,v 1.11 2003/10/03 22:02:49 mckinstry Exp $
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
#include "nls.h"
#include "kbd-chooser.h"

/**
 * @brief list of keyboards present
 */
kbd_t *usb_kbd_get (kbd_t *keyboards)
{
	kbd_t *k = xmalloc(sizeof(kbd_t));
	int serr, res, mounted_fs = 0 ;

	// Set up default entries.
	k->name = "usb";
	k->description = _("USB keyboard");
	k->fd = -1;
	k->deflt = "mac-usb-us";
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;
	
#if defined (KERNEL_2_6)
	/* In 2.6 series, we can detect keyboard via /proc/bus/input
	 *
	 * FIXME: Write this code	 
	 *   Need to search /proc/bus/input/devices
	 
	 */	
	if (di_check_dir ("/proc/bus/input") >= 0) { // 2.6 kernel
#warning "Kernel 2.6 code not written yet"
	}
#endif
	/* 2.4 code */

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
		res = grep ("/proc/bus/usb/drivers", "keyboard");
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
	return keyboards;	
}

/*
 * TODO:
 * USB keyboards should be able to figure out  a default
 * (read lang/country code from USB code) and set default.
 */
