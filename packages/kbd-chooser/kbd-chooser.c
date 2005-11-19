/**
 * Copyright (C) 2002,2003 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id$
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
#include <sys/sysmacros.h>
#include "nls.h"
#include "xmalloc.h"
#include "kbd-chooser.h"


// TODO Move this into debian-installer.h

#ifndef di_info
#define di_info(format...)   di_log(DI_LOG_LEVEL_INFO, format)
#endif
#ifndef di_debug
#define di_debug(format...)  di_log(DI_LOG_LEVEL_INFO, format)
#endif

extern int loadkeys_wrapper (char *map);	// in loadkeys.y

typedef enum { 
	SERIAL_ABSENT = 0,
	SERIAL_PRESENT = 1,
	SERIAL_UNKNOWN = 2
} sercon_state;


struct debconfclient *
mydebconf_client (void) {
	static struct debconfclient *client = NULL;
	if (client == NULL)
		client = debconfclient_new ();
	return client;
}

int
mydebconf_ask (char *priority, char *template, char **result)
{
	int res;
	struct debconfclient *client = mydebconf_client ();

	debconf_input (client, priority, template);
	res = debconf_go (client);
	if (res != CMD_SUCCESS)
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
	int res = 0;
	struct debconfclient *client = mydebconf_client ();
	if ((res = debconf_get (client, template)))
		return res;

	if (client->value == NULL || (strlen (client->value) == 0))
		res = debconf_set (client, template, strdup (value));
	return res;
}


/**
 * @brief  Ensure a directory is present (and readable)
 */
int check_dir (const char *dirname)
{
	struct stat buf;
	if (stat (dirname, &buf))
		return 0;
	return S_ISDIR(buf.st_mode);
}

/**
 * @brief  Do a grep for a string
 * @return 0 if not found, -errno if error, 
 *   and LINESIZE * (line - 1) + pos, when found at line, pos.
 */
int
grep (const char *file, const char *string)
{
	FILE *fp = fopen (file, "r");
	char buf[LINESIZE];
	char * ret;
	int lines = 0;
	if (!fp)
		return -errno;
	while (!feof (fp))	{
		fgets (buf, LINESIZE, fp);
		if ((ret = strstr (buf, string)) != NULL)	{
			fclose (fp);
			return ((int) (ret - buf) + 1 + (lines * LINESIZE));
		}
		lines ++;
	}
	fclose (fp);
	return 0;
}

/*
 * Helper routines for maplist_select
 */

/**
 * @brief return a default locale name, eg. en_US.UTF-8 (Change C, POSIX to en_US)
 * @return - char * locale name (freed by caller)
 */
char *
locale_get (void)
{
	int ret = 0;

	struct debconfclient *client = mydebconf_client ();
	// languagechooser sets locale of the form xx_YY
	// NO encoding used.

	ret = debconf_get (client, "debian-installer/locale");
	if ((ret == 0) && client->value && (strlen (client->value) > 0))
		return strdup(client->value);
	else
		return strdup("en_US");
}

/**
 * @brief parse a locale into pieces. Assume a well-formed locale name
 *
 */
void
locale_parse (char *locale, char **lang, char **territory, char **modifier, char **charset)
{
	char *und, *at, *dot, *loc = strdup (locale);

	// Each separator occurs only once in a well-formed locale
	und = strchr (loc, '_');
	at = strchr (loc, '@');
	dot = strchr (loc, '.');

	if (und)
	    *und = '\0';
	if (at)
	    *at = '\0';
	if (dot)
	    *dot = '\0';

	*lang = loc;
	*territory = und ? (und + 1) : NULL;
	*modifier  = at  ? (at + 1 ) : NULL;
	*charset   = dot ? (dot + 1) : NULL;
}

/**
 * @brief compare langs list with the preferred locale
 * @param langs: colon-seperated list of locales
 * @return score 0-8
 */
int
locale_list_compare (char *langs)
{
	static char *locale = NULL, *lang1 = NULL, *territory1 = NULL, 
	  *modifier1 = NULL, *charset1 = NULL;
	char *lang2 = NULL, *territory2 = NULL, *charset2 =	 
		NULL, *modifier2 = NULL, buf[LINESIZE], *s, *colon;
	int score = 0, best = -1;

	if (!locale)	{
		locale = locale_get ();
		locale_parse (locale, &lang1, &territory1, &modifier1, &charset1);
	}
	strcpy (buf, langs);
	s = buf;
	while (s)	{
		colon = strchr (s, ':');
		if (colon)
			*colon = '\0';
		locale_parse (s, &lang2, &territory2, &modifier2, &charset2);
		if (!strcmp (lang1, lang2))    {
			score = 3;
			if (territory1  && territory2 && !strcmp (territory1, territory2))  	{
				score+=2;
			}
			// Favour 'generic' locales; ie 'fr' matches 'fr' better
			// than 'fr_BE' does
			if (!territory1 && territory2)
				score--;
			if (territory1 && !territory2)
				score++;
			if (charset1 && charset2 && !strcmp (charset1, charset2))	       	{
				score += 3;	// charset more important than territory
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
	for (mp = maplist->maps ; mp != NULL ; mp  = mp->next)	{
		score = locale_list_compare (mp->langs);
		if (score > best) {
			best = score;
			preferred = mp;
		}
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

	maplist_t *p;

	for (p = maplists; p != NULL; p = p->next)    {
		if (strcmp (p->name, name) == 0)
			break;
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
 * @langs	if non-NULL, match these languages
 */
keymap_t *keymap_get (maplist_t * list, const char *name, const char *langs)
{
	keymap_t *mp;

	for (mp = list->maps ; mp != NULL; mp = mp->next)    {
		if (strcmp (mp->name, name) == 0 &&
		    (!langs || strcmp (mp->langs, langs) == 0))
			break;
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

		map = keymap_get (maplist, tab1 + 1, buf);
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

// Yes, it's bubblesort. But for this size of list, it's efficient
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
 * @brief Get template contents (in the current language).
 * @param template Template name
 * @param type Kind of entry
 * @return the translation, to be freed by caller
 */
char *
template_get(const char *template, const char *type)
{
	struct debconfclient *client = mydebconf_client();

	/* cdebconf auto-translates */
	debconf_metaget(client, template, type);
	return strdup(client->value);
}

/**
 * @brief Get translated description for a given template.
 * @param template Template name
 * @return the translation, to be freed by caller
 */
char *
description_get(const char *template)
{
	return template_get(template, "Description");
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
	const char *subarch = di_system_subarch_analyze();

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
#if defined (DEC_KBD)
	keyboards = dec_kbd_get (keyboards, subarch);
#endif
#if defined (HIL_KBD)
	keyboards = hil_kbd_get (keyboards, subarch);
#endif

	// Did we forget to compile in a keyboard ???
	if (DEBUG && keyboards == NULL) {
		di_error (": No keyboards found\n");
		exit (6);
	}
	// Get the (translated) keyboard names
	for (p = keyboards; p != NULL; p = p->next) {
		sprintf(buf, "kbd-chooser/kbd/%s", p->name);
		p->description = description_get(buf);
	}
	return keyboards;
}

/**
 * @brief set debian-installer/uml-console as to whether we are using a user mode linux console
 * This is then passed via prebaseconfig to base-config
 * @return 1 if present, 0 if absent, 2 if unknown.
 */
sercon_state
check_if_uml_console (void)
{
	sercon_state present = SERIAL_UNKNOWN;
	struct debconfclient *client = mydebconf_client ();

	if (grep("/proc/cpuinfo", "User Mode Linux") > 0)
		present = SERIAL_PRESENT;
	else
		present = SERIAL_ABSENT;

	debconf_set (client, "debian-installer/uml-console", present ? "true" : "false");
	di_info ("Setting debian-installer/uml-console to %s", present ? "true" : "false");
	return present;
}

/**
 * @brief set debian-installer/serial console as to whether we are using a serial console
 * This is then passed via prebaseconfig to base-config
 * @return 1 if present, 0 if absent, 2 if unknown.
 */
sercon_state
check_if_serial_console (void)
{
	sercon_state present = SERIAL_UNKNOWN;
	struct debconfclient *client = mydebconf_client ();
	int fd;
	struct serial_struct sr;
	int rets, ret1, ret2, ret;

	// Some UARTs don't support the TIOCGSERIAL ioctl(), so also
        // try to detect serial console via cmdline
	ret1 = grep("/proc/cmdline","console=ttyS");
	ret2 = grep("/proc/cmdline","console=ttys");
	rets = ret1 > ret2 ? ret1 : ret2;
	ret = grep("/proc/cmdline","console=tty0");

	if ((rets > 0) && (rets > ret)) {
		present = SERIAL_PRESENT;
	} else {
                fd = open ("/dev/console", O_NONBLOCK);
                if (fd == -1)
	                return SERIAL_UNKNOWN;
                present = (ioctl (fd, TIOCGSERIAL, &sr) == 0) ? SERIAL_PRESENT : SERIAL_ABSENT;
		close (fd);
	}
	
	debconf_set (client, "debian-installer/serial-console", present ? "true" : "false");
	di_info ("Setting debian-installer/serial-console to %s", present ? "true" : "false");
	return present;
}

/**
 * @brief  Make sure there are no double values in the arch selection dialog.
 * Because usb-kbd.c adds a keyboard for each keyboard detected and
 * because usb-kbd.c may add keyboard type 'at' in some situations when an
 * usb keyboard is detected, it is possible the types 'usb' and 'at' could
 * be present more than once.
 * The functions add_kbdtype and kbdtype_present test for this situation.
 * FIXME: This could maybe be removed when the problems with keymap types for
 *        usb keyboards have been resolved.
 */

typedef struct kbdtype_s {
	char *name;		// short name of kbd arch
	struct kbdtype_s *next;
} kbdtype_t;

static inline kbdtype_t *add_kbdtype (kbdtype_t *archlist, char *name) {
	kbdtype_t *ka = xmalloc (sizeof(kbdtype_t));
	ka->name = name;
	ka->next = archlist;
	archlist = ka;
	return archlist;
}

int kbdtype_present (kbdtype_t *archlist, char *name) {
	kbdtype_t *ka = NULL;

	for (ka = archlist; ka != NULL; ka = ka->next) {
		if (strcmp (ka->name, name) == 0)
			return TRUE;
	}
	return FALSE;
}

/**
 * @brief  Pick a keyboard, adding it to debconf.
 * @return const char *  - priority of question
 */
char *
keyboard_select (char *curr_arch)
{
	kbd_t *kp = NULL, *preferred = NULL;
	kbdtype_t *archlist = NULL;
	char buf_s[LINESIZE], *s = NULL, buf_t[LINESIZE], *t = NULL, *arch_descr = NULL;
	int choices = 0, first_entry_s = 1, first_entry_t = 1;
	sercon_state sercon;
	sercon_state umlcon;
	struct debconfclient *client = mydebconf_client ();

	/* k is returned by a method if it is preferred keyboard.
	 * For 2.4 kernels, we just select one keyboard. 
	 * In 2.6+ we may have per-keyboard keymaps, and better autodetection
	 * of keyboards present.
	 */

	s = buf_s; t = buf_t;
	// Add the keyboards to debconf
	for (kp = keyboards_get (); kp != NULL; kp = kp->next) {
		di_info ("keyboard type %s: present: %s \n", kp->name,
			kp->present == UNKNOWN ? "unknown ": 
			(kp->present == TRUE ? "true: " : "false" ));
		if ((kp->present != FALSE) &&
		    (kbdtype_present (archlist, kp->name) == FALSE)) {
			choices++;
			s = insert_description (s, kp->name, &first_entry_s);
			t = insert_description (t, kp->description, &first_entry_t);
			archlist = add_kbdtype (archlist, kp->name);
			if (strcmp (PREFERRED_KBD, kp->name) == 0) {
				if ((preferred == NULL) || (preferred->present == UNKNOWN)
				    || (kp->present == TRUE))
					preferred = kp;
			} else {
				if ((preferred == NULL) || 
				    (preferred->present != TRUE && kp->present == TRUE))
					preferred = kp;
			}
		}
	}
	sercon = check_if_serial_console();
	umlcon = check_if_uml_console();
	if (sercon == SERIAL_PRESENT || umlcon == SERIAL_PRESENT) {
		debconf_metaget(client, "kbd-chooser/no-keyboard", "Description");
		arch_descr = strdup(client->value);
		choices++;
		s = insert_description (s, "no-keyboard", &first_entry_s);
		t = insert_description (t, arch_descr, &first_entry_t);
		mydebconf_default_set ("console-tools/archs", "none");
	} else {
		// Add option to skip keyboard configuration (use kernel keymap)
		debconf_metaget(client, "kbd-chooser/skip-config", "Description");
		arch_descr = strdup(client->value);
		choices++;
		s = insert_description (s, "skip-config", &first_entry_s);
		t = insert_description (t, arch_descr, &first_entry_t);
		mydebconf_default_set ("console-tools/archs",  
				      preferred ? preferred->name : "skip-config");
	}
	debconf_subst (client, "console-tools/archs", "KBD-ARCHS", buf_s);
	debconf_subst (client, "console-tools/archs", "KBD-ARCHS-L10N", buf_t);
	free(arch_descr);
	// Set medium priority if current selection is no-keyboard or skip-config
	return ((sercon == SERIAL_PRESENT) || (umlcon == SERIAL_PRESENT) ||
		((preferred && preferred->present == TRUE) &&
		 (strcmp (curr_arch, "skip-config") != 0) &&
		 (strcmp (curr_arch, "no-keyboard") != 0))) ? "low" : "medium";
}

/**
 * @brief   choose a given keyboard
 * @arch    keyboard architecture
 * @keymap  ptr to buffer in which to store chosen keymap name
 * @returns CMD_SUCCESS or CMD_GOBACK, keymap set if SUCCESS
 */

int
keymap_select (char *arch, char *keymap)
{
	char template[50], *ptr;
	kbd_t *kb;
	keymap_t *def;
	int res;

	sprintf (template, "console-keymaps-%s/keymap", arch);

	for (kb = keyboards_get (); kb != NULL; kb = kb->next)
		if (!strcmp (kb->name, arch))
			break;
	if (DEBUG && !kb) {
		di_error ("Keyboard not found\n");
		exit (7);
	}
	// If there is a default keymap for this keyboard, select it
	// This is set if we can actually read preferred type from keyboard,
	// and shouldn't have to ask the question.
	if (kb->deflt) {
		def = keymap_get (maplist_get (arch), kb->deflt, NULL);
		mydebconf_default_set (template, kb->deflt);
	}
	res = mydebconf_ask ( kb->deflt ? "low" : "high", template, &ptr);
	if (res != CMD_SUCCESS)
		return res;
	strcpy (keymap, (strlen (ptr) == 0) ? "none" : ptr);

	return CMD_SUCCESS;
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
	// NOTE: not a typo, using 'true' makes things fail. amck!!!
	debconf_fset (client, "debian-installer/keymap", "seen", "yes");
	loadkeys_wrapper (keymap);
}


int
main (int argc, char **argv)
{
	char *kbd_priority, keymap[LINESIZE], buf[LINESIZE], *arch;
	enum { GOBACK, CHOOSE_ARCH, CHOOSE_KEYMAP, QUIT } state = CHOOSE_ARCH;
	struct debconfclient *client;

	setlocale (LC_ALL, "");
	client = mydebconf_client ();
	di_system_init("kbd-chooser"); // enable syslog

	if (argc == 2) { // keymap may be specified on command-line
		keymap_set (client, argv[1]);
		exit (0);
	}

	debconf_capb (client, "backup");
	debconf_version (client,  2);

	read_keymap_files (KEYMAPLISTDIR);
	arch = buf;
	if (debconf_get (client, "console-tools/archs") == CMD_SUCCESS)
		arch = strdup(client->value);
	kbd_priority = keyboard_select (arch);

	while (state != QUIT)   {
		switch (state)	{
		case GOBACK:
			di_info ("kbdchooser: GOBACK received; leaving");
			exit (10);
		case CHOOSE_ARCH: // First select a keyboard arch.
			if (mydebconf_ask (kbd_priority, "console-tools/archs", &arch)) {
				state = GOBACK;
			} else {
				if (arch == NULL || (strlen (arch) == 0)) {
					di_info ("kbd-chooser: not setting keymap (console-tools/archs not set)");
					state = QUIT;
					break;
				}
				di_info ("kbd-chooser: arch %s selected", arch);
				if ((strcmp (arch, "no-keyboard") == 0) ||
				    (strcmp (arch, "skip-config") == 0))	 {
					di_info ("kbd-chooser: not setting keymap");
					state = QUIT;
					break;
				}
				state  = CHOOSE_KEYMAP;
			}
			break;
			
		case CHOOSE_KEYMAP: // Then a keymap within that arch.
			if (keymap_select (arch, keymap) == CMD_GOBACK) {
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
