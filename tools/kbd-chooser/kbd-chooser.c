/* 
 * Copyright (C) 2002,2003 Alastair McKinstry, <mckinstry@computer.org>
 * Released under the GPL
 *
 * $Id: kbd-chooser.c,v 1.23 2003/04/30 17:51:33 mckinstry Exp $
 */

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
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
#include <locale.h>
#include <sys/ioctl.h>
#include "nls.h"
#include "xmalloc.h"
#include "kbd-chooser.h"


#ifndef PREFERRED_KBD
#define PREFERRED_KBD "none"
#endif

extern int loadkeys_wrapper (char *map); // in loadkeys.y


struct debconfclient *mydebconf_get (void)
{
	static struct debconfclient *client = NULL;
	if (client == NULL)
		client = debconfclient_new ();
	return client;
}

int mydebconf_ask (char *priority, char *template, char **result)
{
	int res;
	struct debconfclient *client = mydebconf_get ();

	// res = client->command (client, "fset", template, "seen", "false", NULL);
	res = client->command (client, "input", priority, template, NULL);
	res = client->command (client, "go", NULL);
	if (res != CMDSTATUS_SUCCESS)
		return res;
	res = client->command (client, "get", template, NULL);
	*result = client->value;
	return res;
}

/**
 * @brief Set a default value for a question if one not already there
 */
int mydebconf_default_set (char *template, char *value)
{
	int res;
	struct debconfclient *client = mydebconf_get ();
	res = client->command (client, "get", template, NULL);
	if (res) return res;

	if (client->value == NULL  || (strlen (client->value) == 0)) {
		res = client->command (client, "set", template, STRDUP (value), NULL);
		if (res) return res;
		res = client->command (client, "fset", template, "seen", "false", NULL);
	} 
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
 * Helper routines for maplist_select
 */

/**
 * @brief return a default locale name, eg. en_US.UTF-8 (Change C, POSIX to en_US)
 * @return - char *, (to be freed by caller)
 */
char *locale_get (void)
{
	struct debconfclient *client = mydebconf_get ();
	// languagechooser sets locale of the form xx_YY
	// NO encoding used.

	
	client->command (client, "get", "debian-installer/locale",  NULL);
	if (client->value && (strlen (client->value) > 0))
		return client->value;
	else
		return "en_US";
}


/**
 * @brief parse a locale into pieces. Assume a well-formed locale name
 *
 */
void locale_parse (char *locale, 
		   char **lang, char **territory, char **charset)
{
	char *und, *at , *loc = STRDUP(locale);

	und = strchr (loc, '_');
	at  = strchr (loc, '@');	
	if (at) {
		*at = '\0';
		*charset = at+1;
	} else
		*charset = NULL;
	
	if (und) {
		*und = '\0';
		*territory = und+1;
	} else 
		*territory = NULL;
	*lang = loc;
}

/**
 * @brief compare langs list with the preferred locale
 * @param langs: colon-seperated list of locales
 * @return score 0-3
 */
int locale_list_compare (char *langs)
{
	static char *locale = NULL, *lang1= NULL, *territory1 = NULL, *charset1 = NULL;
	char *lang2 = NULL, *territory2 = NULL, *charset2 = NULL, buf [LINESIZE], *s, *colon;
	int score = 0, best = -1;

	if (locale == NULL) {
		locale = locale_get ();
		locale_parse (locale, &lang1, &territory1, &charset1);
	}
	STRCPY (buf, langs);
	s = buf;
	while (s) {
		colon = strchr (s, ':');
		if (colon) 
			*colon = '\0';			
		locale_parse (s, &lang2, &territory2, &charset2);
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
	char *s = buf, *t = name;

	*s++ = '[';
	while (*t)  *s++ = *t++;
	*s++ = ']'; *s++ = ' ';
	t = description;
	while (*t)  *s++ = *t++;
	*s = '\0';
	return s;
}
 

/**
 * @brief Sort the maps in a maplist.
 */

void maplist_sort (maplist_t *maplist)
{
	keymap_t *mp, **prev;
	int in_order = 0;

	while (!in_order) {
		// Bubblesort convenient for short list structures.
		in_order = 1;
		prev = &(maplist->maps);
		mp = maplist->maps;
		while (mp) {
			if (mp->next && (strcmp (mp->next->description, mp->description) < 0)) {
				in_order = 0;
				*prev = mp->next;
				mp->next = mp->next->next ; 
				(*prev)->next = mp;
			}
			prev = &(mp->next);
			mp = mp->next;
		}
	}
}

/**
 * @brief Enter a maplist into debconf, picking a default via locale.
 * @param maplist - a maplist (for a given arch, for example)
 */
void maplist_select (maplist_t *maplist)
{
	char buf[4 * LINESIZE], template[LINESIZE], *s= NULL , deflt[LINESIZE];
	keymap_t *mp, *preferred = NULL;
	int score = 0, best = -1;
	struct debconfclient *client = mydebconf_get ();

	maplist_sort (maplist);
	
	mp = maplist->maps;
	while (mp) {
		if (s) {
			STRCPY (s, ", ");
			s += 2;
		} else
			s = buf;
		s = insert_description (s, mp->name, mp->description);
		score = locale_list_compare (mp->langs);
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
	if (best > 0) {
		s = insert_description (deflt, preferred->name, preferred->description);
		*s = '\0';
		mydebconf_default_set (template, deflt);
	}
}


/**
 * @brief	Get a maplist "name", creating if necessary
 * @name	name of arch, eg. "ps2"
 */
maplist_t *maplist_get ( const char *name)
{
static maplist_t *maplists = NULL;

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
maplist_t *maplist_parse_file (const char *name)
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
			map->description = STRDUP (dgettext (maplist, tab2+1));
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
			read_keymap_files (fullname); 
		} else { // Assume a file
			
			/* two types of name allowed (for the moment; )
			 * legacy 'console-keymaps-* names and *.keymaps names
			 */
			if (strncmp (ent->d_name, "console-keymaps-", 16) == 0) 
				STRCPY (p, ent->d_name);
			else 
				strncpy (p, ent->d_name, strchr (ent->d_name, '.') - p);			
			maplist_select (maplist_parse_file (fullname));
		}
	}
	closedir (d);
}

/**
 * @brief Sort keyboards
 */
void keyboards_sort (kbd_t **keyboards)
{
	kbd_t *p = *keyboards, **prev ;
	int in_order = 1;


	// Yes, its bubblesort. But for this size of list, its efficient
	while (!in_order) {
		in_order = 1;
		p = *keyboards;
		prev = keyboards;
		while (p) {
			if (p->next && (strcmp (p->next->description, p->description) < 0)) {
				in_order = 0;
				*prev = p->next;
				p->next = p->next->next;
				(*prev)->next = p;
			}
			prev = &(p->next);
			p = p->next;
		}
	}
}

/**
 * @brief Build a list of the keyboards present on this computer
 * @returns kbd_t list
 */
kbd_t *keyboards_get (void)
{
	static kbd_t *keyboards = NULL;

	if (keyboards != NULL)
		return keyboards;

#if defined (USB_KBD)
	keyboards = usb_kbd_get (keyboards);
#endif
#if defined (AT_KBD)
	keyboards = at_kbd_get (keyboards);
#endif
#if defined (MAC_KBD)
	keyboards = mac_kbd_get (keyboards);
#endif
#if defined (SPARC_KBD)
	keyboards = sparc_kbd_get (keyboards);
#endif
#if defined (ATARI_KBD)
	keyboards = atari_kbd_get (keyboards);
#endif
#if defined (AMIGA_KBD)
	keyboards = amiga_kbd_get (keyboards);
#endif
#if defined (SERIAL_KBD)
	keyboards = serial_kbd_get (keyboards);
#endif

	// Did we forget to compile in a keyboard ???
	if (DEBUG &&  keyboards == NULL) DIE ("No keyboards found");

	keyboards_sort (&keyboards);
	return keyboards;
}


/**
 * @brief set debian-installer/serial console as to whether we are using a serial console
 * This is then passed via prebaseconfig to base-config
 * @return 1 if present, 0 if absent, 2 if unknown.
 */
int check_if_serial_console (void)
{
	int fd;
	struct serial_struct sr;
	int present;
	struct debconfclient *client = mydebconf_get ();

	fd = open("/dev/console", O_NONBLOCK);
	if (fd == -1) 
		return 2;
	present = ( ioctl(fd, TIOCGSERIAL, &sr) == 0) ? 1 : 0 ;
	client->command (client, "set", "debian-installer/serial-console",
			 present ? "yes" : "no" , NULL);
	close (fd);
	/* di_logf ("Setting debian-installer/serial-console to %d", present); */
	return present;
}
		
/**
 * @brief If we aren't sure a kbd is present, add an option not to configure
 * (In the critical-questions only case, this will be the default)
 */
void add_no_keyboard_case (char *s, char **preference)
{
	char template[LINESIZE], *t;
	t = insert_description (template, "none", _("No keyboard to configure"));
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
char *keyboard_select (void)
{
	kbd_t *kp = NULL, *preferred = NULL;
	char buf [LINESIZE], *s = NULL, *preference = NULL, *entry;
	int choices = 0;
	struct debconfclient *client = mydebconf_get ();

	/* k is returned by a method if it is preferred keyboard.
	 * For 2.4 kernels, we just select one keyboard. 
	 * In 2.6+ we may have per-keyboard keymaps, and better autodetection
	 * of keyboards present.
	 */

	// Add the keyboards to debconf
	for (kp = keyboards_get(); kp != NULL ; kp = kp->next) {
		if (kp->present != FALSE) {
			choices++;
			s = s ? ( strcpy (s, ", ") + 2) : buf ;
			entry = s;
			s = insert_description (s, kp->name, kp->description);		       			
			*s = '\0';
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
	if (((preferred == NULL) || (preferred->present == UNKNOWN))
			&& check_if_serial_console()) {
		s = s ? ( strcpy (s, ", ") + 2) : buf ;
		di_log ("Can't tell if kbd present; add no keyboard option\n");
		add_no_keyboard_case (s, &preference );
		choices++;
	}		
	client->command (client, "subst", "console-tools/archs", "choices", buf, NULL);		      
	mydebconf_default_set ("console-tools/archs", preference);
	free (preference);	
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
int keymap_select (char *arch, char *keymap)
{
	char template[50], *ptr, preferred[LINESIZE], *s;
	kbd_t *kb;
	keymap_t *def;
	int res;

	STRCPY (template, "console-data/keymap/");
	STRCPY (template + 20, arch);

	// If there is a default keymap for this keyboard, select it
	for (kb = keyboards_get() ; kb != NULL ; kb = kb->next) 
		if (!strcmp(kb->name, arch))
			break;
	if (DEBUG && !kb) DIE ("Keyboard not found\n");
	// Should we set a default ?
	if (kb->deflt) {
		def = keymap_get (maplist_get (arch), kb->deflt);
		s = insert_description (preferred, def->name, def->description);
		*s = '\0';
		mydebconf_default_set (template, preferred);
	}
	
	res = mydebconf_ask ("medium", template, &ptr);
	if (res != CMDSTATUS_SUCCESS)
		return res;
	keymap = ( strlen(ptr) == 0) ? "none" : extract_name (keymap, ptr);
	
	return CMDSTATUS_SUCCESS;
}	

/**
 *  @brief set the keymap, and debconf database
 */
void keymap_set (struct debconfclient *client, char *keymap)
{
	/* di_logf ("kbd_chooser: setting keymap %s", keymap) */;
	client->command (client, "set", "debian-installer/keymap",
			 keymap, NULL);
	// "seen" Used by scripts to decide not to call us again
	client->command (client, "fset", "debian-installer/keymap",
			 "seen", "yes", NULL);
	loadkeys_wrapper (keymap);

}

int main (int argc, char **argv)
{
	char *kbd_priority, *arch = NULL, keymap[LINESIZE], buf[LINESIZE], *s;
	enum { CHOOSE_ARCH, CHOOSE_KEYMAP } state = CHOOSE_ARCH;
	struct debconfclient *client;
	int res;

	setlocale (LC_ALL, "");
	client = mydebconf_get ();

	// As a form of debugging, allow a keyboard map to 
	// be named on command-line
	if (argc == 2)  {
		keymap_set (client, argv[1]);
		exit (0);
	}

	client->command (client, "capb", "backup", NULL);
	client->command (client, "version", "2.0", NULL);
	client->command (client, "title", N_("Select a Keyboard Layout"), NULL);

	read_keymap_files (KEYMAPLISTDIR);
	check_if_serial_console ();
	kbd_priority = keyboard_select ();

	s = buf;

	while (1) {
		switch (state) {

			// First select a keyboard arch. 
		case CHOOSE_ARCH:				 
			res = mydebconf_ask (kbd_priority, "console-tools/archs", &s);
			if (res) {
				if (res == CMDSTATUS_GOBACK) {
					di_log ("kbdchooser: GOBACK recieved; leaving");
					exit (0);
				} else
					exit (res);
			}
			if (s == NULL || (strlen(s) == 0)) {
				di_log("kbd-chooser: not setting keymap (console-tools/archs not set)");
				exit (0);
			}
			arch = extract_name (xmalloc (LINESIZE), s);
			if (strcmp (arch, "none") == 0) {
				di_log ("kbd-chooser: not setting keymap (kbd == none selected)");
				exit (0);
			}
			state = CHOOSE_KEYMAP;
			break;

			// Then a keymap within that arch.
		case CHOOSE_KEYMAP:
			if (keymap_select (arch, keymap) == CMDSTATUS_GOBACK) {
				state = CHOOSE_ARCH;
				break;
			}
			keymap_set (client, keymap);
			exit (0);
			break;			
		}	
	}
}

