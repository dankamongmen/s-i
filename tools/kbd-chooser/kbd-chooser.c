/* @file  kbd-chooser.c
 * @brief Choose a keyboard.
 *
 * Copyright (C) 2002 Alastair McKinstry, <mckinstry@computer.org>
 * Released under the GPL
 *
 * $Id: kbd-chooser.c,v 1.1 2003/01/19 11:37:56 mckinstry Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h> // Needed ATM for toupper()
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h> 
#include <debian-installer.h>
#include <cdebconf/common.h>
#include <cdebconf/debconfclient.h>
#include "nls.h"
#include "kbd-chooser.h"


kbd_t *keyboards = NULL;
static maplist_t *maplists = NULL;
static struct debconfclient *client;

extern int loadkeys_wrapper (char *map); /* in loadkeys.y */


/**
 * @brief Tell the installer to remove console packages from base install
 */
void unselect_console_packages (void)
{
	// FIXME Write this.
}

char *
my_debconf_input (char *priority, char *template)
{
  printf ("DEBUG: in mdi\n");

  client->command (client, "metaget", template, "description", NULL);
  printf ("DEB: value %s\n", client->value);

	client->command (client, "fset", template, "seen", "false", NULL);
	client->command (client, "input", priority, template, NULL);
	client->command (client, "go", NULL);
	client->command (client, "get", template, NULL);
	printf ("DEBUG: leaving mdi\n");
	return client->value;
}

/*
 * Helper routines for select_keymap
 */

/**
 * @brief return a default locale name, eg. en_US.UTF-8 (Change C, POSIX to en_US)
 * @return - char *, (to be freed by caller)
 */
char *get_locale (void)
{
	char *lang, locale[8];
	lang = getenv ("LANGUAGE");
	
	/* LANGUAGE may be set as a colon seperated list by languagechooser;
	 * for the moment, only take the first language
	 */
	if (lang && strlen (lang) > 0)
		strncpy (locale, lang, 2);
	else
		strncpy (locale, "en", 2);
	locale[2] = '_';
	locale[5] = '\0';
	client->command (client, "get", "debian-installer/country", NULL);
	if (client->value && ( strlen (client->value)  >= 2)) {
		strncpy (locale + 3, client->value, 2);
		// FIXME Workaround for languagechooser bug
		locale[3] = toupper (locale[3]);
		locale[4] = toupper (locale[4]);
	}
	else
		strncpy (locale + 3, "US", 2);
	if (DEBUG) printf ("Using language settings %s \n", locale);
	return STRDUP (locale);
}


/**
 * @brief parse a locale into pieces. Assume a well-formed locale name
 *
 */
void parse_locale (char *locale, 
		   char **lang, char **territory, char **charset)
{
	char *und, *at;

	und = strchr (locale, '_');
	at  = strchr (locale, '@');	
	if (at) {
		*at = '\0';
		*charset = STRDUP (at+1);
	} else
		*charset = NULL;
	
	if (und) {
		*und = '\0';
		*territory = STRDUP (und+1);
	} else 
		*territory = NULL;
	*lang = STRDUP (locale);
}

/**
 * @brief compare langs list with the preferred locale
 * @param langs: colon-seperated list of locales
 * @return score 0-3
 */
int compare_locale_list (char *langs)
{
	static char *locale = NULL, *lang1= NULL, *territory1 = NULL, *charset1 = NULL;
	char *lang2 = NULL, *territory2 = NULL, *charset2 = NULL, buf [LINESIZE], *s, *colon;
	int score = 0, best = 0;

	if (locale == NULL) {
		locale = get_locale ();
		parse_locale (locale, &lang1, &territory1, &charset1);
	}
	STRCPY (buf, langs);
	s = buf;
	while (s) {
		colon = strchr (s, ':');
		if (colon) 
			*colon = '\0';			
		parse_locale (s, &lang2, &territory2, &charset2);
		if (!strcmp (lang1, lang2)) {
			score = 1;
			if (territory1 != NULL && territory2 != NULL && !strcmp (territory1, territory2)) {
				score ++;
			}			
			if (charset1 != NULL && charset2 != NULL && !strcmp (charset1, charset2)) {
				score += 2; // charset more important than territory
			}
		}
		best = MAX (score, best);
		s = colon ? (colon+1) : NULL;
	}
	return best;
}


/**
 * @brief  Insert keymap into buffer in the form "[name] translated_description"
 * @return ptr to char after description.
 */
inline char *insert_description (char *buf, keymap_t *mp)
{
	char *s, * t;

	s = buf;
	t = mp->name;
	*s++ = '[';
	while (*t)  *s++ = *t++;
	*s++ = ']'; *s++ = ' ';
	t = gettext (mp->description);
	while (*t)  *s++ = *t++;
	return s;
}
  
/**
 * @brief Enter a maplist into debconf, picking a default via locale.
 * @param maplist - a maplist (for a given arch, for example)
 */
void select_keymap (maplist_t *maplist)
{
	char buf[4 * LINESIZE], template[LINESIZE], *s= NULL , deflt[LINESIZE];
	keymap_t *mp, *preferred = NULL;
	int score = 0, best = 0;
	
	mp = maplist->maps;
	while (mp) {
		if (s) {
			STRCPY (s, ", ");
			s += 2;
		} else
			s = buf;
		s = insert_description (s, mp);
		score = compare_locale_list (mp->langs);
		if (score > best) {
			best = score;
			preferred = mp;
		}
		mp = mp->next;
	}
	*s = '\0';
	STRCPY (template, "console-data/keymap/");
	STRCPY (template + 20, maplist->name);
	client->command (client, "subst", template, "choices", STRDUP (buf), NULL);	
	// set the default
	client->command (client, "fget", template, "seen", NULL);
	if (strcmp(client->value, "false") == 0) {
		s = insert_description (deflt, preferred);
		*s = '\0';
		client->command (client, "set", template, STRDUP (deflt), NULL);
	}
}


/**
 * @brief    Load the keymap files into memory
 * @warning  No error checking on file contents. Assumed correct by installation checks.
 */
maplist_t *parse_keymap_file (const char *name)
{
	FILE *fp;
	maplist_t *mapfile;
	keymap_t *map , **mp;
	char buf[LINESIZE], *tab1, *tab2, *nl;
	fp = fopen (name, "r");
	if (DEBUG && fp == NULL)
		DIE ("Failed to open %s: %s \n", name, strerror (errno));
	mapfile = NEW (maplist_t);
	if (DEBUG && mapfile == NULL)
		DIE ("malloc failed\n");
	mapfile->next = maplists;
	mapfile->name = STRDUP (name + STRLEN (KEYMAPLISTDIR) + STRLEN ("console-keymaps-") + 1);
	maplists = mapfile;
	mp = &(mapfile->maps);

	while (!feof (fp)) {
		map = NEW (keymap_t);
		if (DEBUG && map == NULL) DIE ("Malloc failed\n");
		
		fgets (buf, LINESIZE, fp);
		tab1 = strchr (buf, '\t');
		if (!tab1)
			break; // end of file
		tab2 = strchr (tab1+1, '\t');
		nl = strchr (tab2, '\n');
		*tab1 = '\0';
		*tab2 = '\0';
		*nl = '\0';
		map->langs = STRDUP (buf);
		map->name =  STRDUP (tab1+1);
		map->description = STRDUP (tab2+1);
		map->next = NULL;
		*mp = map;
		mp = &(map->next);
	}
	fclose (fp);
	return mapfile;
}
		

/**
 * @brief   Read keymap files from /usr/share/console/lists and parse them
 * @warning Assumes files present, readable: this should be guaranteed by the installer dependencies
 */
void read_keymap_files (void)
{
	DIR *d;
	char *p, buf[LINESIZE] = KEYMAPLISTDIR "/";
	struct dirent *ent;

	d = opendir (KEYMAPLISTDIR);
	p = buf + STRLEN (KEYMAPLISTDIR)+1; /* Constant. FIXME */

	if (DEBUG && d == NULL)
		DIE ("Failed to open %s: %s\n", KEYMAPLISTDIR, strerror (errno));
	
	ent = readdir (d);
	while (ent) {
		if (strncmp (ent->d_name, "console-keymaps-", 16) == 0) {
			STRCPY (p, ent->d_name);
			select_keymap (parse_keymap_file (buf));
		}		
		ent = readdir (d);
	}	
}


/**
 * @brief  Pick a keyboard.
 * @return const char *  - priority of question
 */
char *ponder_keyboard_choices (void)
{
	kbd_t *k = NULL, *kp = NULL, *preferred = NULL;
	char buf [LINESIZE], *s = NULL;
	int kboards = 0;

#if defined (USB_KBD)
	k = usb_kbd_get ();
#endif
#if defined (AT_KBD)
	k = at_kbd_get ();
#endif
#if defined (MAC_KBD)
	mac_kbd_get ();
#endif
#if defined (SPARC_KBD)
	sparc_kbd_get ();
#endif
#if defined (ATARI_KBD)
	atari_kbd_get ();
#endif
#if defined (AMIGA_KBD)
	amiga_kbd_get ();
#endif
#if defined (SERIAL_KBD)
	serial_kbd_get ();
#endif

	if (DEBUG &&  keyboards == NULL)
		DIE ("No keyboards found");

	/* k is returned by a method if it is preferred keyboard.
	 * For 2.4 kernels, we just select one keyboard. 
	 * In 2.6+ we may have per-keyboard keymaps, and better autodetection
	 * of keyboards present.
	 */
	if (k->present == TRUE)
		preferred = k;

	kp = keyboards;
	// Add the keyboards to debconf
	while (kp) {
		if (kp->present != FALSE) kboards++;
		if (s) {
			STRCPY (s, ", ");
			s += 2;
		} else
			s = buf;		
		STRCPY (s, gettext (kp->name));
		s += STRLEN (gettext (kp->name));
		// If we _know_ a kbd is present, and are uncertain about the preferred kbd,
		// select the known.
		if (preferred == NULL && kp->present == TRUE)
			preferred = kp;
		kp = kp->next;
	}	
	*(++s) = '\0';
	if (preferred == NULL)
		preferred = k;

	client->command (client, "subst", "console-tools/archs", 
			 "choices", buf, NULL);		      
	
	// Set the default option
	client->command (client, "fget", "console-tools/archs", "seen", NULL);
	if (strcmp(client->value, "false") == 0)
		client->command (client, "set", "console-tools/archs", preferred->name, NULL);

	if (kboards == 0) {
		di_log ("No keyboards found; unselecting console-tools, console-data pkgs\n");
		unselect_console_packages ();
		exit (0);
	}				
		
	// Should we prompt the user?
	if (kboards == 1)		
		return "low";
	return (preferred->present == TRUE) ? "medium" : "high";	
}
	

int main (int argc, char **argv)
{
	char *kbd_priority, *ptr, template[50], keymap[20];
	int len;

	client = debconfclient_new (); 
	client->command (client, "title", "Select a Keyboard Layout", NULL);

	read_keymap_files ();

	// First select a keyboard arch. 
	kbd_priority = ponder_keyboard_choices ();
	ptr = my_debconf_input (kbd_priority, "console-tools/archs");
	printf ("DEBUG: pri %s, ptr %s\n", kbd_priority, ptr);

	// Then a keymap within that arch.
	STRCPY (template, "console-data/keymap/");
	STRCPY (template + 20, ptr);
	
	ptr = my_debconf_input ("medium", template);
	// Choice will be of the form "[name] description". Extract name
	len = (int) (strchr (ptr, ']') - ptr) -1 ;	
	strncpy (keymap, ptr+1, len);
	keymap [ len ] = '\0';
	if (DEBUG) printf ("DEBUG: choice %s\n", keymap);
	
	loadkeys_wrapper (keymap);       
       		
	exit (0);
}

