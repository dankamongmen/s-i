/* @file efi-reader.c
 * 
 * Read EFI variables and populate debconf database for debian-installer
 * Copyright (C) 2003, Alastair McKinstry <mckinstry@debian.org>
 * Released under the GNU Public License; see file COPYING for details
 *
 * $Id: efi-reader.c,v 1.1 2003/03/08 14:07:58 mckinstry Exp $
 */

#include <linux/efi.h>

#include <debian-installer.h>
#include <cdebconf/common.h>
#include <cdebconf/commands.h>
#include <cdebconf/debconfclient.h>

/* snarfed from linux kernel efivars.c
 */
typedef struct _efi_variable_t {
	efi_char16_t  VariableName[1024/sizeof(efi_char16_t)];
	efi_guid_t    VendorGuid;
	unsigned long DataSize;
	__u8          Data[1024];
	efi_status_t  Status;
	__u32         Attributes;
} __attribute__((packed)) efi_variable_t;



/**
 * Get the three-letter lang code from /proc
 * return 1 if error, 0 on success
 */
int get_efi_lang_code (char *lang_code)
{
	int fd, err, sz;
	efi_variable_t var_data;
	char *result;

	/* Snarfed variable def. from linux/arch/ia64/kernel/efivars.c
	 * Probably should check out a more official interface, in case the format
	 * of /proc changes.
	 */
	fd = open ("/proc/efi/vars/Lang-8be4df61-93ca-11d2-aa0d-00e098032b8c");
	if (fd < 0) {
		err = errno;
		di_log ("Failed to open /proc/efi/vars/Lang-*");
		di_log (perror (errno));
		return 1;
	}
	sz = read (fd, (void *) var_data, sizeof (efi_variable_t));
	close (fd);
	if (sz == sizeof (efi_variable_t)) { // success
		strncpy (lang_code, var_data.Data, 3);
		return 0;
	}
	return 1;
}

/**
 * locales prefer the language to be a two-letter code;
 * translate three-letter -> two-letter if possible (eg eng -> en)
 */
char *two_code (char *three_code)
{
	int t = 0;
	while (trans_table[t].threecode != '\0' ) {
		if (strcmp (argv[1], trans_table[t].threecode) == 0) 
			return trans_table[t].twocode;
		t++;
	}
	return three_code;
}

/* Read certain variables from EFI and populate debian-installer, as required.
 * At the moment, just read Lang"
 * Check out the 
 */
int main (int argc, char *argv[])
{
	char lang_code[4];
	static struct denconfclient *client;

	client = debconfclient_new ();
	if (get_efi_lang_code[lang_code]) 
		exit (1);
	client->command (client, "set", "debian-installer/language",
			two_code (lang_code), NULL);

}  


/*
 * Local Variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 */
   
