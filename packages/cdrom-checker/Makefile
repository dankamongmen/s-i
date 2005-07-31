# cdrom-checker - verify debian cdrom's 
# Copyright (C) 2003 Thorsten Sauter <tsauter@gmx.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#


CC = gcc
CFLAGS = -W -Wall -Os -fomit-frame-pointer
LD = gcc
LDLFAGS =
LDLIBS = -ldebconfclient -ldebian-installer
APP = cdrom-checker

all: ${APP}

cdrom-checker: main.o
	${LD} ${LDFLAGS} -o ${APP} $? ${LDLIBS}

main.o: main.c main.h
	${CC} ${CFLAGS} -c main.c

strip: all
	strip --remove-section=.comment --remove-section=.note ${APP}

clean:
	rm -f *.o

distclean: clean
	rm -f ${APP}

