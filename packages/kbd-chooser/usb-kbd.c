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
static kbd_t *usb_preferred_keymap (kbd_t *keyboards, const char *subarch)
{
	/* FIXME
	 * It was a mistake to tie "keymaps" to "architectures": all the keymaps
	 * in console-keymaps-usb are Mac-specific (at the moment); "PC" USB keyboards
	 * all use standard "AT" keymaps. But its too close to sarge release to change design,
	 * so we go with the following hack:
	 * If the USB keyboard vendor is Apple, set PRESENT = TRUE.
	 * For other keyboard vendors and if architecture is i386 or powerpc (prep and chrp),
	 * force the installer to display the list of AT keymaps. This is needed because, for
	 * 2.6 kernels, we can not assume that a AT connector will be detected in at-kbd.c.
	 */
	kbd_t *p;
	usb_data *data;
	int usb_present = 0;
	for (p = keyboards; p != NULL; p = p->next) {
		if (strcmp(p->name,"usb") == 0) {
//			usb_present = 1;
			data = (usb_data *) p->data;
			if (data->vendorid == 0x05ac) { // APPLE
				di_debug ("Apple USB keyboard detected\n");
				p->present = TRUE;
			} else {
				di_debug ("non-Apple USB keyboard detected\n");
				p->present = UNKNOWN;     // Is this really an USB/Mac keyboard?
#if defined(__i386__) && defined (AT_KBD)
				di_debug ("Forcing keymap list to AT (i386)\n");
				p->name = "at";           // Force installer to show AT keymaps
				p->present = TRUE;
#endif
#if defined(__powerpc__) && defined (AT_KBD)
				if (strstr (subarch, "mac") == NULL) {
					di_debug ("Forcing keymap list to AT (powerpc)\n");
					p->name = "at";   // Force installer to show AT keymaps
					p->present = TRUE;
				}
#endif
			}
			if (strcmp(p->name,"usb") == 0)
				usb_present = 1;
		}
	}
	// Ensure at least 1 USB entry
	if (!usb_present) {
		di_debug ("Adding generic entry for USB keymaps\n");
		p = usb_new_entry (keyboards);
		keyboards = p;
	}
	return keyboards;
}

/**
 * @brief parse /proc/bus/usb/devices, looking for keyboards
 */
static kbd_t *usb_parse_proc (kbd_t *keyboards)
{
	usb_data *data = NULL;
	kbd_t *k = NULL;
	char buf[LINESIZE], *p;
	FILE *fp;
	int mounted_fs = 0;
	int serr, vendorid = 0, productid = 0;

	fp = fopen ("/proc/bus/usb/devices", "r");
	if (fp == NULL) {	// try harder.
		di_debug ("Mounting usbdevfs to look for kbd\n");
		// redirect stderr for the moment
		serr = dup(2);
		close (2);
		open ("/dev/null", O_RDWR);
		if (system ("mount -t  usbdevfs usbdevfs /proc/bus/usb") != 0) {
			return keyboards; // ok, now you can give up.
		}
		// restore stderr
		close (2);
		dup (serr);
		mounted_fs = 1;
		fp = fopen("/proc/bus/usb/devices", "r");
	}
	if (fp) {
		di_debug ("Parsing /proc/bus/usb/devices\n");
		while (!feof(fp)) {
			fgets(buf, LINESIZE, fp);
			if ((p = strstr (buf, "Vendor=")) != NULL) {
				vendorid = DEHEX(p + 7);
				p = strstr(buf, "ProdID=");
				productid = DEHEX(p + strlen("ProdID="));
			}
			if ((p = strstr(buf, "usbkbd")) != NULL) {
					// This stanza refers to a usbkbd. We can use the
					// latest Vendor=XXXX ProdID=XXXX results
					di_debug ("Found usbkbd kdb: 0x%hx:0x%hx\n", vendorid, productid);
					k = usb_new_entry (keyboards);
					data = xmalloc(sizeof(usb_data));
					k->data = (usb_data *) data;
					data->vendorid = vendorid;
					data->productid = productid;
					k->present = TRUE;
					keyboards = k;
			}
			if (((p = strstr(buf, "usbhid")) != NULL) &&
			    ((p = strstr(buf, "Cls=03")) != NULL) &&    // Human Interface Device
			    ((p = strstr(buf, "Sub=01")) != NULL) &&    // Boot Interface Subclass
			    ((p = strstr(buf, "Prot=01")) != NULL))  {  // Keyboard
					// This stanza refers to a usbhid with a keyboard
					// attached. Unfortunately AFAICT there's no info
					// on the keyboard itself. For now let's assume
					// we can use the Vendor & ProdID of the HID itself.
					di_debug ("Found usbhid kbd: 0x%hx:0x%hx\n", vendorid, productid);
					k = usb_new_entry (keyboards);
					data = xmalloc(sizeof(usb_data));
					k->data = (usb_data *) data;
					data->vendorid = vendorid;
					data->productid = productid;
					k->present = TRUE;
					keyboards = k;
			}
		}
		fclose(fp);
	} else {
		di_debug ("Failed to open /proc/bus/usb/devices: %d\n", errno);
	}
	if (mounted_fs)
		system ("umount /proc/bus/usb");

	return keyboards;
}


/**
 * @brief list of keyboards present
 * @return at least 1 "usb" entry.
 */
kbd_t *usb_kbd_get (kbd_t *keyboards, const char *subarch)
{

#if defined(__MIPSEL__)
        // DECstations do not have USB keyboards
         if (strstr(subarch, "r3k-kn02") || strstr(subarch,"r4k-kn04"))
               return keyboards;
#endif

	// Find all USB keyboards via /proc/bus/usb/devices
	keyboards = usb_parse_proc (keyboards);
	// Mark the default keymaps for each USB keyboard
	keyboards = usb_preferred_keymap (keyboards, subarch);

	return keyboards;
}
