/* @file  usb-kbd.c
 * @brief Report keyboards present on PC
 *
 * Copyright (C) 2002 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: usb-kbd.c,v 1.2 2003/01/19 12:23:31 mckinstry Exp $
 */

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <debian-installer.h>
#include <cdebconf/common.h>  // for NEW()
#include "nls.h"
#include "kbd-chooser.h"

extern kbd_t *keyboards;	/* in kbd-chooser.c */

/**
 * @brief  Do a grep for a string
 * @return 0 if present, 1 if not
 */
int grep (const char *file, const char *string)
{
	FILE *fp = fopen (file, "r");	
	char buf[LINESIZE];
	if (DEBUG && fp == NULL)
		DIE ("Failed to open %s to search for %s\n", file, string);
	while (!feof (fp)) {
		fgets (buf, LINESIZE, fp);
		if (strstr (buf, string) != NULL) {
			fclose (fp);
			return 0;
		}
	}
	fclose (fp);
	return 1;
}

/**
 * @brief list of keyboards present
 */
kbd_t *usb_kbd_get (void)
{
	kbd_t *k = NEW (kbd_t);
	int err, mounted_fs = 0 ;
	struct stat sbuf;

	k->name = "usb";
	k->description = N_("USB");
	k->fd = -1;
	k->deflt = NULL;
	
#if defined (KERNEL_2_5)
	/* In 2.5 series, we can detect keyboard via /proc/bus/input
	 *
	 * FIXME: Write this code	 
	 */	
#warning "Kernel 2.5 code not written yet"
	k->present = UNKNOWN ;

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
	if ((err = stat ("/proc/bus/usb/drivers", &sbuf)) < 0) { /* assume ENOENT */
		if (DEBUG) {
			perror ("Looking for /proc/bus/usb/drivers: ");
			di_log ("No keyboard drivers file");
			if (DEBUG) { 
				if (stat ("/proc/bus/usb", &sbuf) < 0) 
					DIE ("reading /proc/bus/usb failed. ");
			}
		}
		err = system ("mount -t  usbdevfs usbdevfs /proc/bus/usb");
		if (err != 0)
			DIE ("Failed to mount USB filesystem");
		mounted_fs = 1;
	}
	if (grep ("/proc/bus/usb/drivers", "keyboard") == 0) {
		di_log ("Found USB Keyboard\n");
		k->present = TRUE;		
		k->next = keyboards;
		keyboards = k;
	} else {
		k->present = FALSE;
	}	
	if (mounted_fs)
		system ("umount /proc/bus/usb");

#endif
	return k;
}

/*
 * TODO:
 * USB keyboards should be able to figure out  a default
 * (read lang/country code from USB code) and set default.
 */
