#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file copyright in the debian subdirectory of this archive
# for more details.
#
# Copyright (C) 2001 by Gerhard Tonn and Stefan Gybas
#

CC := gcc
CFLAGS := -Wall -W -pipe -Os -I/usr/include/sysfs
LDFLAGS := -ldebconfclient -ldebian-installer -lsysfs

ifneq (,$(findstring debug,$(DEB_BUILD_OPTIONS)))
CFLAGS += -g
else
CFLAGS += -fomit-frame-pointer
endif

all: postinst

postinst: netdevice.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f postinst

