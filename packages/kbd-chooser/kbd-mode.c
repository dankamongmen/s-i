/*
 * Set the console mode.
 * Adapted from the console-tools.
 * 
 * Copyright (C) 2004  Recai Oktas <roktas@omu.edu.tr>
 *
 * This file is part of the Debian-installer (kbd-chooser module).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#include <stdio.h>
#include <getopt.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <debian-installer.h>
#include "getfd.h"

#ifndef di_info
#define di_info(format...) di_log(DI_LOG_LEVEL_INFO, format)
#endif

extern int getfd (void);

static char *progname = "kbd-mode";

static void usage (void)
{
	printf ("Usage: %s [OPTION]...\n", progname);
	puts   ("Report and set keyboard mode");
	puts   ("");
	puts   ("Options are:");
	puts   ("-h --help       print this help information and exit");
	puts   ("-a --8bit       ASCII or 8bit mode (XLATE)");
	puts   ("-k --keycode    keycode mode (MEDIUMRAW)");
	puts   ("-u --unicode    UTF-8 mode (UNICODE)");
	puts   ("-s --scancode   scancode mode (RAW)");
}

static int parse_cmdline (int argc, char *argv[])
{
	int mode;
	int c;
	const struct option long_opts[] = {
		{ "help"     , no_argument, NULL, 'h' },
		{ "8bit"     , no_argument, NULL, 'a' },
		{ "ascii"    , no_argument, NULL, 'a' },
		{ "keycode"  , no_argument, NULL, 'k' },
		{ "scancode" , no_argument, NULL, 's' },
		{ "unicode"  , no_argument, NULL, 'u' },
		{ NULL, 0, NULL, 0 }
	};
	
	mode = -1;
	while ( (c = getopt_long (argc, argv, "haksu", long_opts, NULL)) != EOF)
		switch (c) {
			case 'h':
				usage ();
				exit(0);
			case 'a':
				mode = K_XLATE;
				break;
			case 'u':
				mode = K_UNICODE;
				break;
			case 's':
				mode = K_RAW;
				break;
			case 'k':
				mode = K_MEDIUMRAW;
				break;
		}

	return mode;
}

static const char *desc (int mode)
{
	switch (mode) {
		case K_RAW:
			return "raw (scancode)";
		case K_MEDIUMRAW:
			return "mediumraw (keycode)";
		case K_XLATE:
			return "ASCII (default)";
		case K_UNICODE:
			return "Unicode (UTF-8)";
		default:
			return "unknown";
	}
}

int main (int argc, char *argv[])
{
	int fd;
	int mode;

	mode = parse_cmdline (argc, argv);

	fd = getfd ();
	
	/* Report console mode.  */
	if (mode == -1)	{
		if (ioctl (fd, KDGKBMODE, &mode)) {
			di_error ("%s: error reading console mode\n", progname);
			exit (1);
		}

		printf ("Console mode: %s\n", desc (mode));

		exit (0);
	}

	/* Set console mode.  */
	if (ioctl (fd, KDSKBMODE, mode)) {
		di_error ("%s: error setting console mode to %s\n", progname, desc (mode));
		exit (1);
	}
	else {
		di_info ("%s: setting console mode to %s\n", progname, desc (mode));
	}

	exit (0);
}
