#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file copyright in the debian subdirectory of this archive
# for more details.
#
# Copyright (C) 2001 by Gerhard Tonn and Stefan Gybas
#

CC := gcc
CFLAGS := -Wall -W -pipe -Os
LDFLAGS := -ldebconfclient -ldebian-installer

ifneq (,$(findstring debug,$(DEB_BUILD_OPTIONS)))
CFLAGS += -g
STRIP = /bin/true
else
CFLAGS += -fomit-frame-pointer
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment
endif

all: netdevice

netdevice: netdevice.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@

clean:
	rm -f netdevice

