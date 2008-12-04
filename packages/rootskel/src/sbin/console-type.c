/*
 * Licensed under GPLv2
 *
 * Adapted for Debian Installer by Frans Pop <fjp.debian.org> from
 * cttyhack from busybox 1.11, which is
 *
 * Copyright (c) 2007 Denys Vlasenko <vda.linux@googlemail.com>
 */

#include <sys/ioctl.h>
#include <stdio.h>

enum { VT_GETSTATE = 0x5603 }; /* get global vt state info */

int main(int argc, char ** argv)
{
	/*
	 * Use an (oversized) dummy buffer as we're not interested in
	 * returned values.
	 * TIOCGSERIAL normally uses serial_struct from <linux/serial.h>
	 * VT_GETSTATE normally uses vt_stat from <linux/vt.h>
	 */
	char buffer[1024]; /* filled by ioctl */

	if (ioctl(0, TIOCGSERIAL, buffer) == 0) {
		/* this is a serial console */
		printf("serial\n");
	} else if (ioctl(0, VT_GETSTATE, buffer) == 0) {
		/* this is linux virtual tty */
		printf("virtual\n");
	}

	return 0;
}
