#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file copyright in the debian subdirectory of this archive
# for more details.
#
# Copyright (C) 2001 by Gerhard Tonn and Stefan Gybas
#

CC := gcc
CFLAGS := -Wall -Werror -pipe -O2

all: changrep

changrep: changrep.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f changrep

