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
#include <sys/utsname.h>
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
	 * For other keyboard vendors and if architecture is x86 or powerpc (prep and chrp),
	 * force the installer to display the list of AT keymaps. This is needed because, for
	 * 2.6 kernels, we can not assume that a AT connector will be detected in at-kbd.c.
	 *
	 * UPDATE
	 * Because of the changes in the input layer, we can now be sure that an
	 * AT keyboard layout is needed, even if an USB keyboard is detected. So we force
	 * any USB keyboard to AT and no longer include the option to select a USB keymap.
	 */

	// Always use AT keymaps for USB keyboards with 2.6 kernel
	int skip_26_kernels = 0;
#if defined (AT_KBD)
	struct utsname buf;
	uname(&buf);
	if (strncmp(buf.release, "2.6", 3) == 0)
		skip_26_kernels = 1;
#endif

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
#if defined(__powerpc__) && defined (AT_KBD)
				if (strstr (subarch, "mac") == NULL) {
					di_debug ("Forcing keymap list to AT (powerpc)\n");
					p->name = "at";   // Force installer to show AT keymaps
					p->present = TRUE;
				}
#endif
			}
			if ((strcmp(p->name,"usb") == 0) && (skip_26_kernels)) {
				di_debug ("Forcing keymap list to AT (2.6 kernel)\n");
				p->name = "at";           // Force installer to use AT keymap
				p->present = TRUE;
			}
			if (strcmp(p->name,"usb") == 0)
				usb_present = 1;
		}
	}
	// Ensure at least 1 USB entry, unless the arch uses AT for 2.6 kernels
	if ((!usb_present) && (!skip_26_kernels)) {
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
		if (system ("mount -t  usbfs usbfs /proc/bus/usb") != 0) {
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
			// The combination of Cls=03, Sub=01, Prot=01 seems a very reliable
			// test to see that an usb keyboard is present. The Driver=...
			// information varies (hid, usbkbd, usbhid) and is ignored
			if (((p = strstr(buf, "Cls=03")) != NULL) &&    // Human Interface Device
			    ((p = strstr(buf, "Sub=01")) != NULL) &&    // Boot Interface Subclass
			    ((p = strstr(buf, "Prot=01")) != NULL))  {  // Keyboard
					di_debug ("Found usb keyboard: 0x%hx:0x%hx\n", vendorid, productid);
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
