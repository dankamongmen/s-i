/**
 * Copyright (C) 2002,2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id: kbd-chooser.c,v 1.34 2003/10/31 22:55:54 mckinstry Exp $
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
#include <cdebconf/debconfclient.h>
#include <linux/serial.h>
#include <locale.h>
#include <sys/ioctl.h>
#include "nls.h"
#include "xmalloc.h"
#include "kbd-chooser.h"

// TODO These should be in cdebconf
#define CMDSTATUS_SUCCESS 	0
#define CMDSTATUS_GOBACK	30

// TODO Move this into debian-installer.h
#define di_info(format...)   di_log(DI_LOG_LEVEL_INFO, format)
#define di_debug(format...)  di_log(DI_LOG_LEVEL_DEBUG, format)

extern int loadkeys_wrapper (char *map);	// in loadkeys.y


struct debconfclient *
mydebconf_get (void) {
	static struct debconfclient *client = NULL;
	if (client == NULL)
		client = debconfclient_new ();
	return client;
}

int
mydebconf_ask (char *priority, char *template, char **result)
{
	int res;
	struct debconfclient *client = mydebconf_get ();

	debconf_input (client, priority, template);
	res = debconf_go (client);
	if (res != CMDSTATUS_SUCCESS)
		return res;
	res = debconf_get (client, template);
	*result = client->value;
	return res;
}

/**
 * @brief Set a default value for a question if one not already there
 */
int
mydebconf_default_set (char *template, char *value)
{
	int res;
	struct debconfclient *client = mydebconf_get ();
	res = debconf_get (client, template);
	if (res)
		return res;

	if (client->value == NULL || (strlen (client->value) == 0))
	{
		res = debconf_set (client, template, strdup (value));
		if (res)
			return res;
	}
	return res;
}

/**
 * @brief  Do a grep for a string
 * @return 0 if present, 1 if not, -errno if error
 */
int
grep (const char *file, const char *string)
{
	FILE *fp = fopen (file, "r");
	char buf[LINESIZE];
	if (!fp)
		return -errno;
	while (!feof (fp))	{
		fgets (buf, LINESIZE, fp);
		if (strstr (buf, string) != NULL)	{
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
char *
locale_get (void)
{
	struct debconfclient *client = mydebconf_get ();
	// languagechooser sets locale of the form xx_YY
	// NO encoding used.

	debconf_get (client, "debian-installer/locale");
	if (client->value && (strlen (client->value) > 0))
		return client->value;
	else
		return "en_US";
}

/**
 * @brief parse a locale into pieces. Assume a well-formed locale name
 *
 */
void
locale_parse (char *locale, char **lang, char **territory, char **charset)
{
	char *und, *at, *loc = strdup (locale);

	und = strchr (loc, '_');
	at = strchr (loc, '@');
	if (at)	{
		*at = '\0';
		*charset = at + 1;
	}   else
		*charset = NULL;

	if (und)	{
		*und = '\0';
		*territory = und + 1;
	} else {
		*territory = NULL;
	}
	*lang = loc;
}

/**
 * @brief compare langs list with the preferred locale
 * @param langs: colon-seperated list of locales
 * @return score 0-4
 */
int
locale_list_compare (char *langs)
{
	static char *locale = NULL, *lang1 = NULL, *territory1 = NULL, *charset1 =
		NULL;
	char *lang2 = NULL, *territory2 = NULL, *charset2 =
		NULL, buf[LINESIZE], *s, *colon;
	int score = 0, best = -1;

	if (!locale)	{
		locale = locale_get ();
		locale_parse (locale, &lang1, &territory1, &charset1);
	}
	strcpy (buf, langs);
	s = buf;
	while (s)	{
		colon = strchr (s, ':');
		if (colon)
			*colon = '\0';
		locale_parse (s, &lang2, &territory2, &charset2);
		if (!strcmp (lang1, lang2))    {
			score = 2;
			if (territory1 != NULL && territory2 != NULL
			    && !strcmp (territory1, territory2))  	{
				score++;
			}
			// Favour 'generic' locales; ie 'fr' matches 'fr' better
			// than 'fr_BE' does
			if (territory1 == NULL && territory2 != NULL)
				score--;
			if (charset1 != NULL && charset2 != NULL
			    && !strcmp (charset1, charset2))	       	{
				score += 2;	// charset more important than territory
			}
		}
		best = (score > best) ? score : best;
		s = colon ? (colon + 1) : NULL;
	}
	return best;
}


/**
 * @brief  Insert description into buffer
 * @description ; may be NULL.
 * @return      ptr to char after description.
 */
char *
insert_description (char *buf, char *description, int *first_entry)
{
	char *s = buf;

	if (*first_entry) {
		*first_entry = 0;
	} else {
		strcpy (s, ", ");
		s += 2;
	}
	strcpy (s, description);
	s += strlen (description);
	*s = '\0';
	return s;
}


/**
 * @brief Enter a maplist into debconf, picking a default via locale.
 * @param maplist - a maplist (for a given arch, for example)
 */
void
maplist_select (maplist_t * maplist)
{
	char template[LINESIZE];
	keymap_t *mp, *preferred = NULL;
	int score = 0, best = -1;

	// Pick the default
	mp = maplist->maps;
	while (mp)	{
		score = locale_list_compare (mp->langs);
		if (score > best)      	{
			best = score;
			preferred = mp;
		}
		mp = mp->next;
	}
	if (best > 0)	{
		sprintf (template, "console-keymaps-%s/keymap", maplist->name);
		mydebconf_default_set (template, preferred->name);
	}
}


/**
 * @brief	Get a maplist "name", creating if necessary
 * @name	name of arch, eg. "at", "mac"
 */
maplist_t *maplist_get (const char *name)
{
	static maplist_t *maplists = NULL;

	maplist_t *p = maplists;

	while (p)    {
		if (strcmp (p->name, name) == 0)
			break;
		p = p->next;
	}
	if (p)
		return p;
	p = di_new (maplist_t,1);
	if (p == NULL)    {
		di_error (": Failed to create maplist (out of memory)\n");
		exit (1);
	}
	p->next = maplists;
	p->maps = NULL;
	p->name = strdup (name);
	maplists = p;
	return p;
}

/**
 * @brief	Get a keymap in a maplist; create if necessary
 * @list	maplist to search
 * @name	name of list
 */
keymap_t *keymap_get (maplist_t * list, char *name)
{
	keymap_t *mp = list->maps;

	while (mp)    {
		if (strcmp (mp->name, name) == 0)
			break;
		mp = mp->next;
	}
	if (mp)
		return mp;
	mp = di_new (keymap_t,1);
	if (mp == NULL)    {
		di_error (": Failed to malloc keymap_t");
		exit (2);
	}
	mp->langs = NULL;
	mp->name = strdup (name);
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
maplist_t *
maplist_parse_file (const char *name)
{
	FILE *fp;
	maplist_t *maplist;
	keymap_t *map;
	char buf[LINESIZE], *tab1, *tab2, *nl;
	fp = fopen (name, "r");

	if (fp == NULL) {
		di_error (": Failed to open %s: %s \n", name, strerror (errno));
		exit (3);
	}
	maplist = maplist_get ((char *) (name + strlen (KEYMAPLISTDIR) +
				       strlen ("console-keymaps-") + 1));

	while (!feof (fp))   {
		fgets (buf, LINESIZE, fp);
		if (*buf == '#')		//comment ; skip line
			continue;
		tab1 = strchr (buf, '\t');
		if (!tab1)
			continue;		// malformed line
		tab2 = strchr (tab1 + 1, '\t');
		if (!tab2)
			continue;		// malformed line
		nl = strchr (tab2, '\n');
		if (!nl)
			continue;		// malformed line
		*tab1 = '\0';
		*tab2 = '\0';
		*nl = '\0';

		map = keymap_get (maplist, tab1 + 1);
		if (!map->langs) {	// new keymap
			map->langs = strdup (buf);
			map->description = strdup (tab2 + 1);
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
void
read_keymap_files (char *listdir)
{
	DIR *d;
	char *p, fullname[LINESIZE];
	struct dirent *ent;
	struct stat sbuf;

	strncpy (fullname, listdir, LINESIZE);
	p = fullname + strlen (listdir);
	*p++ = '/';

	d = opendir (listdir);
	if (d == NULL)	{
		di_error (": Failed to open %s: %s (keymap files probably not installed)\n",
			 listdir, strerror (errno));
		exit (4);
	}
	ent = readdir (d);
	for (; ent; ent = readdir (d))	{
		if ((strcmp (ent->d_name, ".") == 0) ||
		    (strcmp (ent->d_name, "..") == 0))
			continue;
		strcpy (p, ent->d_name);
		if (stat (fullname, &sbuf) == -1)		{
			di_error (": Failed to stat %s: %s\n", fullname,
				 strerror (errno));
			exit (5);
		}
		if (S_ISDIR (sbuf.st_mode))	{
			read_keymap_files (fullname);
		}	else	{				// Assume a file
			/* two types of name allowed (for the moment; )
			 * legacy 'console-keymaps-* names and *.keymaps names
			 */
			if (strncmp (ent->d_name, "console-keymaps-", 16) == 0) {
				strcpy (p, ent->d_name);
				maplist_select (maplist_parse_file (fullname));
			}
		}
	}

	closedir (d);
}
 
/**
 * @brief Sort keyboards
 */
void
keyboards_sort (kbd_t ** keyboards)
{
	kbd_t *p = *keyboards, **prev;
	int in_order = 1;

// Yes, its bubblesort. But for this size of list, its efficient
	while (!in_order) {
		in_order = 1;
		p = *keyboards;
		prev = keyboards;
		while (p) {
			if (p->next && 
			    (strcmp (p->next->description, p->description) < 0)) {
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
 * @brief Get translated contents of a given template.
 * based on code from main-menu. 
 * TODO: merge into the cdebconf library someday
 * FIXME: Cope with sublangs : eg de_CH, de ?
 */
char *
translated_template_get(char *template)
{
	int ret = 0;
	static char *language = NULL;
	struct debconfclient *client = mydebconf_get();
	
	if (!language) {
		 ret = debconf_get(client,"debian-installer/language");
		 if (client->value && (ret != 10))
			 language = strdup(client->value);
	}
	if (language) {
		char field[128];
		snprintf(field, sizeof (field), "Description-%s.UTF-8", language);
		if (!debconf_metaget(client, template, field)) 
			return (strdup(client->value));
	}
	// Description must exist.
	debconf_metaget(client, template, "Description");
        return strdup(client->value);
}

/**
 * @brief discover what subarchitecture we have
 */
const char *subarch_get (void)
{
	static char *subarch = NULL;

	if (!subarch) {
		struct debconfclient *client = mydebconf_get();

		if (debconf_get (client, "debian-installer/kernel/subarchitecture"))
			subarch = "";
		else
			subarch = strdup(client->value);
	}
	return subarch;
}

/**
 * @brief Build a list of the keyboards present on this computer
 * @returns kbd_t list
 */
kbd_t *
keyboards_get (void)
{
	static kbd_t *keyboards = NULL, *p = NULL;
	char buf[25];
	const char *subarch = subarch_get();

	if (keyboards != NULL)
		return keyboards;

#if defined (USB_KBD)
	keyboards = usb_kbd_get (keyboards, subarch);
#endif
#if defined (AT_KBD)
	keyboards = at_kbd_get (keyboards, subarch);
#endif
#if defined (MAC_KBD)
	keyboards = mac_kbd_get (keyboards, subarch);
#endif
#if defined (SPARC_KBD)
	keyboards = sparc_kbd_get (keyboards, subarch);
#endif
#if defined (ATARI_KBD)
	keyboards = atari_kbd_get (keyboards, subarch);
#endif
#if defined (AMIGA_KBD)
	keyboards = amiga_kbd_get (keyboards, subarch);
#endif
#if defined (SERIAL_KBD)
	keyboards = serial_kbd_get (keyboards, subarch);
#endif

	// Did we forget to compile in a keyboard ???
	if (DEBUG && keyboards == NULL) {
		di_error (": No keyboards found\n");
		exit (6);
	}
	// translate the keyboard names
	p = keyboards;
	while (p) { 
		sprintf(buf, "kbd-chooser/kbd/%s", p->name);
		p->description = translated_template_get(buf);
		p = p->next;
	}
	keyboards_sort (&keyboards);
	return keyboards;
}

/**
 * @brief translate localised keyboard name back to kbd. arch name
 */
char *keyboard_parse (char *reply)
{
	kbd_t *kb;
	for (kb = keyboards_get(); kb != NULL; kb = kb->next) {
		if (!strcmp (reply, kb->description))
			break;
	}
	return (kb) ? kb->name : "none";
}

/**
 * @brief set debian-installer/serial console as to whether we are using a serial console
 * This is then passed via prebaseconfig to base-config
 * @return 1 if present, 0 if absent, 2 if unknown.
 */
int
check_if_serial_console (void)
{
	int fd;
	struct serial_struct sr;
	int present;
	struct debconfclient *client = mydebconf_get ();

	fd = open ("/dev/console", O_NONBLOCK);
	if (fd == -1)
		return 2;
	present = (ioctl (fd, TIOCGSERIAL, &sr) == 0) ? 1 : 0;
	debconf_set (client,  "debian-installer/serial-console", present ? "yes" : "no");
	close (fd);

	di_info ("Setting debian-installer/serial-console to %d", present);
	return present;
}

/**
 * @brief  Pick a keyboard, adding it to debconf.
 * @return const char *  - priority of question
 */
char *
keyboard_select (void)
{
	kbd_t *kp = NULL, *preferred = NULL;
	char buf[LINESIZE], *s = NULL, *none = NULL;
	int choices = 0, first_entry = 1;
	struct debconfclient *client = mydebconf_get ();

	/* k is returned by a method if it is preferred keyboard.
	 * For 2.4 kernels, we just select one keyboard. 
	 * In 2.6+ we may have per-keyboard keymaps, and better autodetection
	 * of keyboards present.
	 */

	s = buf;
	// Add the keyboards to debconf
	for (kp = keyboards_get (); kp != NULL; kp = kp->next) {
		if (kp->present != FALSE) {
			choices++;
			s = insert_description (s,  kp->description, &first_entry);
			if (strcmp (PREFERRED_KBD, kp->name) == 0) {
				if ((preferred == NULL) || (preferred->present == UNKNOWN)
				    || (kp->present == TRUE))
					preferred = kp;
			} else {
				if (preferred == NULL || (preferred->present != TRUE))
					preferred = kp;
			}
		}
	}
	if (((preferred == NULL) || (preferred->present == UNKNOWN))
	    && check_if_serial_console ()) {
		di_info ("Can't tell if kbd present; add no keyboard option\n");
		none = translated_template_get ("kbd-chooser/no-keyboard");
		s = insert_description (s, none, &first_entry);
		choices++;
	}
	debconf_subst (client, "console-tools/archs", "choices", buf);
	mydebconf_default_set ("console-tools/archs",  
			      preferred ? preferred->description : none);
	if (none)
		free(none);
	return (preferred->present == TRUE) ? "low" : "medium";
}

/**
 * @brief   choose a given keyboard
 * @arch    keyboard architecture
 * @keymap  ptr to buffer in which to store chosen keymap name
 * @returns CMDSTATUS_SUCCESS or CMDSTATUS_GOBACK, keymap set if SUCCESS
 */

/* FIXME FIXME REVIEW */
int
keymap_select (char *arch, char *keymap)
{
	char template[50], *ptr;
	kbd_t *kb;
	keymap_t *def;
	int res;

	sprintf (template, "console-keymaps-%s/keymap", arch);

	// If there is a default keymap for this keyboard, select it
	for (kb = keyboards_get (); kb != NULL; kb = kb->next)
		if (!strcmp (kb->name, arch))
			break;
	if (DEBUG && !kb) {
		di_error ("Keyboard not found\n");
		exit (7);
	}
	// Should we set a default ?
	if (kb->deflt) {
		di_debug ( "keymap_select: setting default %s", kb->deflt);
		def = keymap_get (maplist_get (arch), kb->deflt);
		mydebconf_default_set (template, kb->description);
	}
	res = mydebconf_ask ("medium", template, &ptr);
	if (res != CMDSTATUS_SUCCESS)
		return res;
	strcpy (keymap, (strlen (ptr) == 0) ? "none" : ptr);

	di_debug ("keymap_select: ptr %s", keymap);
	return CMDSTATUS_SUCCESS;
}

/**
 *  @brief set the keymap, and debconf database
 */
void
keymap_set (struct debconfclient *client, char *keymap)
{
	di_info ("kbd_chooser: setting keymap %s", keymap);
	debconf_set (client, "debian-installer/keymap", keymap);
	// "seen" Used by scripts to decide not to call us again
	debconf_fset (client, "debian-installer/keymap", "seen", "yes");
	loadkeys_wrapper (keymap);

}


int
main (int argc, char **argv)
{
	char *kbd_priority, *arch = NULL, keymap[LINESIZE], buf[LINESIZE], *s;
	enum { GOBACK, CHOOSE_ARCH, CHOOSE_KEYMAP, QUIT } state = CHOOSE_ARCH;
	struct debconfclient *client;

	setlocale (LC_ALL, "");
	client = mydebconf_get ();
	di_system_init("kbd-chooser"); // enable syslog

	if (argc == 2) { // keymap may be specified on command-line
		keymap_set (client, argv[1]);
		exit (0);
	}

	debconf_capb (client, "backup");
	debconf_version (client,  2);

	read_keymap_files (KEYMAPLISTDIR);
	check_if_serial_console ();
	kbd_priority = keyboard_select ();
	s = buf;

	while (state != QUIT)   {
		switch (state)	{
		case GOBACK:
			di_info ("kbdchooser: GOBACK recieved; leaving");
			exit (30);
		case CHOOSE_ARCH: // First select a keyboard arch.
			if (mydebconf_ask (kbd_priority, "console-tools/archs", &s)) {
				state = GOBACK;
			} else {
				if (s == NULL || (strlen (s) == 0)) {
					di_info ("kbd-chooser: not setting keymap (console-tools/archs not set)");
					state = QUIT;
					break;
				}
				arch = keyboard_parse(s);
				if (strcmp (arch, "none") == 0)	 {
					di_info ("kbd-chooser: not setting keymap (kbd == none selected)");
					state = QUIT;
				}
			}
			state  = CHOOSE_KEYMAP;
			break;
			
		case CHOOSE_KEYMAP: // Then a keymap within that arch.
			if (keymap_select (arch, keymap) == CMDSTATUS_GOBACK) {
				state = CHOOSE_ARCH;
			} else {
				di_info ("choose_keymap: keymap = %s", keymap);
				keymap_set (client, keymap);
				state = QUIT;
			}
			break;
		case QUIT:
			break;
		}
	}
	exit (0);
}
