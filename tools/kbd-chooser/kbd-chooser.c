/* @file  kbd-chooser.c
 * @brief Choose a keyboard.
 * 
 * Copyright (C) 2002 Alastair McKinstry, <mckinstry@computer.org>
 * Released under the GPL
 *
 * $Id: kbd-chooser.c,v 1.5 2003/01/28 11:02:05 mckinstry Exp $
 */

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h> // Needed ATM for toupper()
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h> 
#include <debian-installer.h>
#include <cdebconf/common.h>
#include <cdebconf/commands.h>
#include <cdebconf/debconfclient.h>
#include "nls.h"
#include "xmalloc.h"
#include "kbd-chooser.h"

kbd_t *keyboards = NULL;
static maplist_t *maplists = NULL;
static struct debconfclient *client;

extern int loadkeys_wrapper (char *map); // in loadkeys.y


int my_debconf_input (char *priority, char *template, char **result)
{
	int res;
	client->command (client, "fset", template, "seen", "false", NULL);
	client->command (client, "input", priority, template, NULL);
	res = client->command (client, "go", NULL);
	if (res != CMDSTATUS_SUCCESS)
		return res;
	printf ("(DEBUG4)\n");
	res = client->command (client, "get", template, NULL);
	printf ("(DEBUG5)\n");
	*result = client->value;
	return res;
}

/**
 * @brief  Do a grep for a string
 * @return 0 if present, 1 if not, errno if error
 */
int grep (const char *file, const char *string)
{
	FILE *fp = fopen (file, "r");	
	char buf[LINESIZE];
	if (!fp)
		return errno;
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

/*
 * Helper routines for select_keymap
 */

/**
 * @brief return a default locale name, eg. en_US.UTF-8 (Change C, POSIX to en_US)
 * @return - char *, (to be freed by caller)
 */
char *get_locale (void)
{
	char *lang, locale[LINESIZE];
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
inline char *insert_description (char *buf, char *name, char *description)
{
	char *s, * t;

	s = buf;
	t = name;
	*s++ = '[';
	while (*t)  *s++ = *t++;
	*s++ = ']'; *s++ = ' ';
	t = gettext (description);
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
		s = insert_description (s, mp->name, mp->description);
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
	client->command (client, "subst", template, "choices", buf, NULL);	
	// set the default
	client->command (client, "fget", template, "seen", NULL);
	if (strcmp(client->value, "false") == 0) {
		s = insert_description (deflt, preferred->name, preferred->description);
		*s = '\0';
		client->command (client, "set", template, deflt, NULL);
	}
}


/**
 * @brief    Load the keymap files into memory
 @ @name     keymap filename.
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
	mapfile->next = maplists;
	mapfile->name = STRDUP (name + STRLEN (KEYMAPLISTDIR) + STRLEN ("console-keymaps-") + 1);
	maplists = mapfile;
	mp = &(mapfile->maps);

	while (!feof (fp)) {
		map = NEW (keymap_t);
		
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
	p = buf + STRLEN (KEYMAPLISTDIR)+1; 

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

void collect_keyboards (void)
{

#if defined (USB_KBD)
	usb_kbd_get ();
#endif
#if defined (AT_KBD)
	at_kbd_get ();
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
}


char  *add_no_keyboard_case (char *s, char **preferred_name)
{
	char template[256], *t;
	t = insert_description (template, "none", N_("No keyboard to configure"));
	*(++t) = '\0';
	strcpy (s, t);
	*preferred_name = STRDUP (t);

	return s + strlen (t);
}

char *extract_name (char *name, char *ptr)
{
	int len;
	// Choice will be of the form "[name] description". Extract name
	len = (int) (strchr (ptr, ']') - ptr) -1 ;	
	strncpy (name, ptr+1, len);
	name[len] = '\0';
	return name;
}

/**
 * @brief  Pick a keyboard.
 * @return const char *  - priority of question
 */
char *ponder_keyboard_choices (void)
{
	kbd_t *kp = NULL, *preferred = NULL;
	char buf [LINESIZE], *s = NULL, *preferred_arch;
	int kboards = 0, res;

	assert (maplists != NULL); // needed to choose default kbd in some cases

	collect_keyboards ();

	// Did we forget to compile in a keyboard ???
	if (DEBUG &&  keyboards == NULL)
		DIE ("No keyboards found");

	/* k is returned by a method if it is preferred keyboard.
	 * For 2.4 kernels, we just select one keyboard. 
	 * In 2.6+ we may have per-keyboard keymaps, and better autodetection
	 * of keyboards present.
	 */

	// Add the keyboards to debconf
	for (kp = keyboards; kp != NULL ; kp = kp->next) {
		if (kp->present != FALSE) {
			kboards++;
			if (s) {
				STRCPY (s, ", ");
				s += 2;
			} else
				s = buf;		
			s = insert_description (s, kp->name, kp->description);
			if (preferred == NULL || (preferred->present != TRUE))
				preferred = kp;					
		}	
	}
	if ((preferred == NULL) || (preferred->present == UNKNOWN)) 
		s = add_no_keyboard_case (s, &preferred_arch);
	else 
		preferred_arch = preferred->name;
	
	// Set the default option
	res = client->command (client, "fget", "console-tools/archs", "seen", NULL);
	if (strcmp(client->value, "false") == 0) {
	  printf ("DEBUG6: val %s preferred %s , res %d\n", client->value, preferred_arch, res);
		client->command (client, "set", "console-tools/archs", strdup (preferred_arch), NULL);
	}
	*s = '\0';
	client->command (client, "subst", "console-tools/archs", 
			 "choices", buf, NULL);		      
	       
	// Should we prompt the user?
	if (kboards <= 1)		
		return "low";
	return (preferred->present == TRUE) ? "medium" : "high";	
}

/**
 * @brief   choose a given keyboard
 * @arch    keyboard architecture
 * @keymap  ptr to buffer in which to store chosen keymap
 * @returns CMDSTATUS_SUCCESS or CMDSTATUS_GOBACK, keymap set if SUCCESS
 */
int choose_keymap (char *arch, char *keymap)
{
	char template[50], *ptr, preferred[LINESIZE], *s;
	kbd_t *kb;
	int res;
	STRCPY (template, "console-data/keymap/");
	STRCPY (template + 20, arch);

	// If there is a default keymap for this keyboard, select it
	for (kb = keyboards ; kb != NULL ; kb = kb->next) 
		if (!strcmp(kb->name, arch))
			break;
	if (DEBUG && !kb)
		DIE ("Keyboard not found\n");
	if (kb->deflt) {
		s = insert_description (preferred, kb->deflt->name, kb->deflt->description);
		*s = '\0';
		client->command (client, "set", template, preferred, NULL);
	}
	
	res = my_debconf_input (kb->deflt ? "low" : "medium", 
				template, &ptr);
	if (res != CMDSTATUS_SUCCESS)
		return res;

	keymap = extract_name (keymap, ptr);
	
	return CMDSTATUS_SUCCESS;
}	


int main (int argc, char **argv)
{
	char *kbd_priority, *arch = NULL, keymap[LINESIZE], buf[LINESIZE], *s;
	enum { CHOOSE_ARCH, CHOOSE_KEYMAP } state = CHOOSE_ARCH;

	client = debconfclient_new (); 
	client->command (client, "capb", "backup", NULL);
	client->command (client, "title", "Select a Keyboard Layout", NULL);

	read_keymap_files ();

	kbd_priority = ponder_keyboard_choices ();

	s = buf;

	while (1) {

		switch (state) {

			// First select a keyboard arch. 
		case CHOOSE_ARCH:				  
			if (my_debconf_input (kbd_priority, "console-tools/archs", &s) != CMDSTATUS_SUCCESS)
				exit (0);
			arch = extract_name (xmalloc (LINESIZE), s);
			if (strcmp (arch, "none") == 0)
				exit (0);
			state = CHOOSE_KEYMAP;
			break;

			// Then a keymap within that arch.
		case CHOOSE_KEYMAP:
			if (choose_keymap (arch, keymap) == CMDSTATUS_GOBACK) {
				state = CHOOSE_ARCH;
				break;
			}
			client->command (client, "set", "console-data/keymap", keymap, NULL);
			loadkeys_wrapper (keymap);  
			exit (0);
			break;			
		}	
	}
}

