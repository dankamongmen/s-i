/* @file kbd-chooser.h
 * 
 * Copyright (C) 2002 Alastair McKinstry   <mckinstry@computer.org>  
 * Released under the GNU License; see file COPYING for details 
 * 
 * $Id: kbd-chooser.h,v 1.12 2003/11/14 20:53:16 mckinstry Rel $
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
	char *description;	/* description  */
	char *deflt;	/* default keymap for this keyboard */
	exists present;
	int fd;			
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

extern int grep (const char *file, const char *string);
extern int check_dir (const char *dirname);

#ifdef SPARC_KBD
#define PREFERRED_KBD "sparc"
#endif

#ifdef AT_KBD
#define PREFERRED_KBD "at"
#endif

#ifndef PREFERRED_KBD
#define PREFERRED_KBD "none"
#endif

#endif  /* KBD_CHOOSER_H */
