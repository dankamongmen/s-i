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

// Define this if you want 2.5 kernel support (as well as 2.4)
#undef KERNEL_2_5

#endif
