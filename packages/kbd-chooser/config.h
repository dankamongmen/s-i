/* config.h for kbd-chooser
 *
 *  
 */


#ifndef _CONFIG_H
#define _CONFIG_H

// For a debug build, define DEBUG=1. 
#if !defined(DEBUG)
#define DEBUG 0
#define NDEBUG 1
#endif

#define DATADIR "/usr/share"
#define KEYMAPDIR "keymaps"
#define DEFMAP "us"

#define PROGNAME "kbd-chooser"

// #undef ENABLE_NLS

// Define this if you want 2.5 kernel support

// #define KERNEL_2_5 1
#define KERNEL_2_4


// Uncomment these to add more charset support.
// For console-data, = 2002.12.04dbs-06, only
// iso-8859-[1289] are required.

// #define CHARSET_ETHIOPIC 1
// #define CHARSET_SAMI 1
// #define CHARSET_ISO_8859_3  1
#define CHARSET_ISO_8859_4  1
#define CHARSET_ISO_8859_5  1
// #define CHARSET_ISO_8859_7  1

#define CHARSET_ISO_8859_8
#define CHARSET_ISO_8859_9


// Sanity checks

#if !defined(KERNEL_2_4) && !defined(KERNEL_2_5)
#error "Support for a kernel must be compiled in"
#endif

#endif
