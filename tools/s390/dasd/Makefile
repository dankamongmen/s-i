#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file copyright in the debian subdirectory of this archive
# for more details.
#
# Copyright (C) 2001 by Gerhard Tonn and Stefan Gybas
#

CC := gcc
CFLAGS := -Wall -pipe -Os
#LDFLAGS := -ldebconf
LDFLAGS := /usr/lib/libdebconf.a

all: dasd

dasd: dasd.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f dasd

