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


// Quick 4 digit hex -> integer conversion
#define DEHEX(s) ( ((*(s) - '0') << 12) + ((*(s+1) -'0') << 8) + ((*(s+2)-'0') << 4) + ((*(s+3)-'0')))

// Arch-specific information about USB Keyboards
typedef struct usb_data_s { 
	uint16_t vendorid;
	uint16_t productid;
} usb_data;


static inline kbd_t *usb_new_entry (kbd_t *keyboards) {
	kbd_t *k = xmalloc (sizeof(kbd_t));
	k->name = "usb";
	k->data = NULL;
	k->deflt = NULL;
	k->present = UNKNOWN;
	k->next = keyboards;
	return k;
}


/**
 * @brief  Pick the best keymap for given USB keyboards.
 */
static kbd_t *usb_preferred_keymap (kbd_t *keyboards)
{
	/* FIXME
	 * It was a mistake to tie "keymaps" to "architectures": all the keymaps
	 * in console-keymaps-usb are Mac-specific (at the moment); "PC" USB keyboards
	 * all use standard "AT" keymaps. But its too close to sarge release to change design,
	 * so we go with the following hack:
	 * If the USB keyboard vendor is Apple, set PRESENT = TRUE.
	 */
	kbd_t *p;
	usb_data *data;
	int usb_present = 0;
	for (p = keyboards; p != NULL; p = p->next) {
		if (strcmp(p->name,"usb") == 0) {
			usb_present = 1;
			data = (usb_data *) p->data;
			if (data->vendorid == 0x05ac) // APPLE
				p->present = TRUE;
			else
				p->present = FALSE;
		}
	}
	// Ensure at least 1 USB entry
	if (!usb_present)
		keyboards = usb_new_entry (keyboards);
	return keyboards;
}

/**
 * @brief parse /proc/bus/usb/devices, looking for keyboards
 */
static void usb_parse_proc (kbd_t *keyboards)
{
	usb_data *data = NULL;
	kbd_t *k = NULL;
	char buf[LINESIZE], *p;
	FILE *fp;
	int mounted_fs = 0;
	int serr, vendorid = 0, productid = 0;

	fp = fopen ("/proc/bus/usb/devices", "r");
	if (fp == NULL) {	// try harder.
		di_debug ("mounting usbdevfs to look for kbd");		
		// redirect stderr for the moment
		serr = dup(2);
		close (2);
		open ("/dev/null", O_RDWR);
		if (system ("mount -t  usbdevfs usbdevfs /proc/bus/usb") != 0) {
			return; // ok, now you can give up.
		}
		// restore stderr
		close (2);
		dup (serr);
		mounted_fs = 1;
		fp = fopen("/proc/bus/usb/devices", "r");
	}
	if (fp) {
		while (!feof(fp)) {
			fgets(buf, LINESIZE, fp);
			if ((p = strstr (buf, "Vendor=")) != NULL) {
				vendorid = DEHEX(p + 7);
				p = strstr(buf, "ProdID=");
				productid = DEHEX(p + strlen("ProdID="));
			}
			if ((p = strstr(buf, "usbkbd")) != NULL) {
					// This stanza refers to a usbkbd. we can use the
					// latest Vendor=XXXX ProdID=XXXX results
					k = usb_new_entry (keyboards);
					data = xmalloc(sizeof(usb_data));
					k->data = (usb_data *) data;
					data->vendorid = vendorid;
					data->productid = productid;
					k->present = TRUE;
			}
		}
		fclose(fp);
	} else {
		di_debug ("Failed to open /proc/bus/usb/devices: %d\n", errno);
	}
	if (mounted_fs)
		system ("umount /proc/bus/usb");
}


/**
 * @brief list of keyboards present
 * @return at least 1 "usb" entry.
 */
kbd_t *usb_kbd_get (kbd_t *keyboards, const char *subarch)
{

#if defined(__mipsel__)
        // DECstations do not have USB keyboards
         if (strstr(subarch, "r3k-kn02") || strstr(subarch,"r4k-kn04"))
               return keyboards;
#endif

	// Find all USB keyboards via /proc/bus/usb/devices
	usb_parse_proc (keyboards);	
	// Mark the default keymaps for each USB keyboard
	keyboards = usb_preferred_keymap (keyboards);

	return keyboards;
}
