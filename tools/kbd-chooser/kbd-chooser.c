/* @brief Choose a keyboard.
 * 
 * Copyright (C) 2002 Alastair McKinstry, <mckinstry@computer.org>
 * Released under the GPL
 *
 * $Id: kbd-chooser.c,v 1.12 2003/03/05 21:03:35 mckinstry Exp $
 */

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h> // Needed ATM for toupper()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <errno.h> 
#include <debian-installer.h>
#include <cdebconf/common.h>
#include <cdebconf/commands.h>
#include <cdebconf/debconfclient.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include "nls.h"
#include "xmalloc.h"
#include "kbd-chooser.h"


#ifndef PREFERRED_KBD
#define PREFERRED_KBD "none"
#endif


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
	res = client->command (client, "get", template, NULL);
	*result = client->value;
	return res;
}

/**
 * @brief  Do a grep for a string
 * @return 0 if present, 1 if not, -errno if error
 */
int grep (const char *file, const char *string)
{
	FILE *fp = fopen (file, "r");	
	char buf[LINESIZE];
	if (!fp)
		return -errno;
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
	// languagechooser sets locale of the form xx_YY
	// NO encoding used.
	
	client->command (client, "get", "debian-installer/locale",  NULL);
	if (client->value && (strlen (client->value) > 0))
		return STRDUP (client->value);
	else
		return STRDUP ("en_US");
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
	*s = '\0';
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
 * @brief	Get a maplist "name", creating if necessary
 */
maplist_t *maplist_get (char *name)
{
	maplist_t *p = maplists;
	
	while (p) {
		if (strcmp(p->name, name) == 0)
			break;
		p = p->next;
	}	
	if (p) return p;
	p = NEW (maplist_t);
	if (DEBUG && p==NULL)
	  DIE ("Failed to create maplist\n");
	p->next = maplists;
	p->maps = NULL;
	p->name = STRDUP (name);
	maplists = p;
	return p;
}

/**
 * @brief	Get a keymap in a maplist; create if necessary
 * @list	maplist to search
 * @name	name of list
 */
keymap_t *keymap_get (maplist_t *list, char *name)
{
	keymap_t *mp = list->maps;

	while (mp) { 
	  if (strcmp (mp->name, name) == 0)
			break;
		mp = mp->next;
	}
	if (mp)  
	  return mp;
	mp = NEW (keymap_t);
	if (DEBUG && mp == NULL)
	  DIE ("Failed to malloc keymap_t");
	mp->langs = NULL;
	mp->name = STRDUP (name);
	mp->description = NULL;
	mp->next = list->maps;
	list->maps = mp;
	return mp;
}


/**
 * @brief    Load the keymap files into memory
 @ @name     keymap filename.
 * @warning  No error checking on file contents. Assumed correct by installation checks.
 */
maplist_t *parse_keymap_file (const char *name)
{
	FILE *fp;
	maplist_t *maplist;
	keymap_t *map;
	char buf[LINESIZE], *tab1, *tab2, *nl;
	fp = fopen (name, "r");

	if (DEBUG && fp == NULL)
		DIE ("Failed to open %s: %s \n", name, strerror (errno));
	maplist = maplist_get ((char *) (name + STRLEN (KEYMAPLISTDIR) + STRLEN ("console-keymaps-") + 1));

	while (!feof (fp)) {
		fgets (buf, LINESIZE, fp);
		if (*buf == '#' ) //comment ; skip line
			continue; 
		tab1 = strchr (buf, '\t');
		if (!tab1)
			continue; // malformed line
		tab2 = strchr (tab1+1, '\t');
		if (!tab2)
			continue; // malformed line
		nl = strchr (tab2, '\n');
		if (!nl)
			continue; // malformed line
		*tab1 = '\0';
		*tab2 = '\0';
		*nl = '\0';
		
		map = keymap_get (maplist, tab1+1);
		if (! map->langs) { // new keymap
			map->langs = STRDUP (buf);
			map->description = STRDUP (tab2+1);
		}
	}
	fclose (fp);
	return maplist;
}
		

/**
 * @brief   Read keymap files from /usr/share/console/lists and parse them
 * @listdir Directory to look in
 * @warning Assumes files present, readable: this should be guaranteed by the installer dependencies
 */
void read_keymap_files (char *listdir)
{
	DIR *d;
	char *p, fullname[LINESIZE];
	struct dirent *ent;
	struct stat sbuf;

	strncpy (fullname, listdir, LINESIZE); 
	p = fullname + STRLEN (listdir);
	*p++ = '/';

	d = opendir (listdir);

	if (DEBUG && d == NULL)
		DIE ("Failed to open %s: %s\n", listdir, strerror (errno));
	
	ent = readdir (d);
	for (; ent ; ent = readdir (d)) {
		if ((strcmp (ent->d_name, ".") == 0) ||
		    (strcmp (ent->d_name, "..") == 0))
			continue;
		strcpy (p, ent->d_name);
		if (stat (fullname, &sbuf) == -1) {
			if (DEBUG) 			       
				DIE ("Failed to stat %s: %s\n", fullname,
				     strerror (errno));
			// otherwise continue
			continue;
		}
		if (S_ISDIR (sbuf.st_mode)) {
			read_keymap_files (p); 
		} else { // Assume a file
			
			/* two types of name allowed (for the moment; )
			 * legacy 'console-keymaps-* names and *.keymaps names
			 */
			if (strncmp (ent->d_name, "console-keymaps-", 16) == 0) 
				STRCPY (p, ent->d_name);
			 else 
				 strncpy (p, ent->d_name, strchr (ent->d_name, '.') - p);			
			select_keymap (parse_keymap_file (fullname));
		}
	}
	closedir (d);
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

/**
 * @brief set debian-installer/serial console as to whether we are using a serial console
 * This is then passed via prebaseconfig to base-config
 */
void check_if_serial_console (void)
{
	int fd;
	struct serial_struct sr;
	char *present;

	fd = open("/dev/console", O_NONBLOCK);
	if (fd == -1) 
		return;
	present = ( ioctl(fd, TIOCGSERIAL, &sr) == 0) ? "yes" : "no" ;
	client->command (client, "set", "debian-installer/serial-console",
			 present, NULL);
	close (fd);
}
		
/**
 * @brief If we aren't sure a kbd is present, add an option not to configure
 * (In the critical-questions only case, this will be the default)
 */
void add_no_keyboard_case (char *s, char **preference)
{
	char template[LINESIZE], *t;
	t = insert_description (template, "none", N_("No keyboard to configure"));
	*t = '\0';
	strcpy (s, template);
	if (*preference == NULL)
		*preference = STRDUP (template);
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
	char buf [LINESIZE], *s = NULL, *preference = NULL, *entry;
	int choices = 0, res;

	assert (maplists != NULL); // needed to choose default kbd in some cases

	collect_keyboards ();

	// Did we forget to compile in a keyboard ???
	if (DEBUG &&  keyboards == NULL) DIE ("No keyboards found");

	/* k is returned by a method if it is preferred keyboard.
	 * For 2.4 kernels, we just select one keyboard. 
	 * In 2.6+ we may have per-keyboard keymaps, and better autodetection
	 * of keyboards present.
	 */

	// Add the keyboards to debconf
	for (kp = keyboards; kp != NULL ; kp = kp->next) {
		if (kp->present != FALSE) {
			choices++;
			s = s ? ( strcpy (s, ", ") + 2) : buf ;
			entry = s;
			s = insert_description (s, kp->name, kp->description);		       			
			if (strcmp (PREFERRED_KBD, kp->name) == 0) {
				preference = strdup (entry);
				if ((preferred == NULL) || (preferred->present ==UNKNOWN) || (kp->present == TRUE))
					preferred = kp;
			} else {
				if (preferred == NULL || (preferred->present != TRUE))
					preferred = kp;					
			}
		}	
	}	
	if ((preferred == NULL) || (preferred->present == UNKNOWN)) {
		s = s ? ( strcpy (s, ", ") + 2) : buf ;
		di_log ("Setting no keybd case\n");
		add_no_keyboard_case (s, &preference );
		choices++;
	}		
	client->command (client, "subst", "console-tools/archs", 
			 "choices", buf, NULL);		      
	
	// Set the default option
	res = client->command (client, "fget", "console-tools/archs", "seen", NULL);
	if (strcmp(client->value, "false") == 0) {
		client->command (client, "set", "console-tools/archs", preference, NULL);
	}
	
	// Should we prompt the user?
	if (choices < 2)		
		return "low";
	return (preferred->present == TRUE) ? "low" : "medium";	
}

/**
 * @brief   choose a given keyboard
 * @arch    keyboard architecture
 * @keymap  ptr to buffer in which to store chosen keymap name
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

	if (strlen (ptr) == 0)
		keymap = STRDUP ("none");
	else
		keymap = extract_name (keymap, ptr);
	
	return CMDSTATUS_SUCCESS;
}	


int main (int argc, char **argv)
{
	char *kbd_priority, *arch = NULL, keymap[LINESIZE], buf[LINESIZE], *s;
	enum { CHOOSE_ARCH, CHOOSE_KEYMAP } state = CHOOSE_ARCH;
	int res;
	
	// As a form of debugging, allow a keyboard map to 
	// be named on command-line
	if (argc == 2) {
		loadkeys_wrapper (argv[1]);
		exit (0);
	}

	client = debconfclient_new (); 
	client->command (client, "capb", "backup", NULL);
	client->command (client, "title", "Select a Keyboard Layout", NULL);

	read_keymap_files (KEYMAPLISTDIR);

	check_if_serial_console ();
	kbd_priority = ponder_keyboard_choices ();

	s = buf;

	while (1) {

		switch (state) {

			// First select a keyboard arch. 
		case CHOOSE_ARCH:				 
			res = my_debconf_input (kbd_priority, "console-tools/archs", &s);
			if (res != CMDSTATUS_SUCCESS) {
				exit (res == CMDSTATUS_GOBACK ? 0 : 1);
			}

			arch = extract_name (xmalloc (LINESIZE), s);
			if (strcmp (arch, "none") == 0) {
				di_log ("not setting keymap");
				exit (0);
			}
			state = CHOOSE_KEYMAP;
			break;

			// Then a keymap within that arch.
		case CHOOSE_KEYMAP:
			if (choose_keymap (arch, keymap) == CMDSTATUS_GOBACK) {
				state = CHOOSE_ARCH;
				break;
			}
			client->command (client, "set", "debian-installer/keymap", 
					 keymap, NULL);
			loadkeys_wrapper (keymap);  
			exit (0);
			break;			
		}	
	}
}

