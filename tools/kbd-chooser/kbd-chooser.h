/* @file kbd-chooser.h
 * 
 * Copyright (C) 2002 Alastair McKinstry   <mckinstry@computer.org>  
 * Released under the GNU License; see file COPYING for details 
 * 
 * $Id: kbd-chooser.h,v 1.6 2003/01/28 11:02:36 mckinstry Exp $
 */

#ifndef KBD_CHOOSER_H
#define KBD_CHOOSER_H

#define DEFMAP "us" /* Default Map */
#define KEYMAPDIR "keymaps"
#define KEYMAPLISTDIR "/usr/share/console/lists"

#define LINESIZE 512

typedef enum { TRUE = 1, FALSE = 0, UNKNOWN = -1 } exists;

typedef struct keymap_s {
	char *langs;
	char *name;
	char *description;
	struct keymap_s *next;
} keymap_t;

typedef struct maplist_s {
	char *name;
	keymap_t *maps;
	struct maplist_s *next;
} maplist_t;

typedef struct kbd_s { 
	char *name;		/* short name of kbd arch */
	char *description;	/* description in english */
	keymap_t *deflt;	/* default keymap for this keyboard (may be null) */
	exists present;
	int fd;			
	struct kbd_s *next;
} kbd_t;

/* Some of the following will be linked in
 * via *-kbd.c
 */
extern void at_kbd_get (void);
extern void usb_kbd_get (void);
extern void mac_kbd_get (void);
extern void sparc_kbd_get (void);
extern void amiga_kbd_get (void);
extern void serial_kbd_get (void);
extern void atari_kbd_get (void);

extern int grep (const char *file, const char *string);

#endif
