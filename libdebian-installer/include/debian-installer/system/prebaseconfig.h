/*
 * prebaseconfig.h
 *
 * Copyright (C) 2003 Bastian Blank <waldi@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: prebaseconfig.h,v 1.1 2003/08/29 12:37:33 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__SYSTEM__PREBASECONFIG_H
#define DEBIAN_INSTALLER__SYSTEM__PREBASECONFIG_H

#define DI_PREBASECONFIG_DIR "/usr/lib/prebaseconfig.d"

int di_prebaseconfig_append (const char *udeb, const char *format, ...);

#endif
