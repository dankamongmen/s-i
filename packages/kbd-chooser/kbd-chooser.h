/* @file kbd-chooser.h
 * 
 * Copyright (C) 2002 Alastair McKinstry   <mckinstry@computer.org>  
 * Released under the GNU License; see file COPYING for details 
 * 
 * $Id$
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
	char *name;		// short name of kbd arch
	char *description;	// description 
	char *deflt;		// default keymap for this keyboard
	exists present;		// Is keyboard present ?
	void *data;		// Arch-specific data goes here
	struct kbd_s *next;
} kbd_t;


/* Some of the following will be linked in
 * via *-kbd.c
 */
extern kbd_t *at_kbd_get (kbd_t *keyboards,const char *subarch);
extern kbd_t *usb_kbd_get (kbd_t *keyboards,const char *subarch);
extern kbd_t *mac_kbd_get (kbd_t *keyboards,const char *subarch);
extern kbd_t *sparc_kbd_get (kbd_t *keyboards,const char *subarch);
extern kbd_t *amiga_kbd_get (kbd_t *keyboards,const char *subarch);
extern kbd_t *serial_kbd_get (kbd_t *keyboards,const char *subarch);
extern kbd_t *atari_kbd_get (kbd_t *keyboards,const char *subarch);
extern kbd_t *dec_kbd_get (kbd_t *keyboards, const char *subarch);
extern kbd_t *hil_kbd_get (kbd_t *keyboards, const char *subarch);

/* Used by *_kb_get() methods */
extern int grep (const char *file, const char *string);
extern int check_dir (const char *dirname);
char *locale_get (void);

#if defined(__sparc__)
#define PREFERRED_KBD "sun"
#endif

#if defined(__m68k___)
#define PREFERRED_KBD "none"
#endif

#if defined(AT_KBD) && !defined(PREFERRED_KBD)
#define PREFERRED_KBD "at"
#endif

#if !defined(PREFERRED_KBD)
#define PREFERRED_KBD "none"
#endif

#endif  /* KBD_CHOOSER_H */
