/*
 * exec.h
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
 * $Id: exec.h,v 1.1 2003/08/29 12:37:33 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__EXEC_H
#define DEBIAN_INSTALLER__EXEC_H

#include <debian-installer/types.h>

/**
 * @defgroup di_exec Exec functions
 * @{
 */

int di_exec (const char *path, const char *const argv[]);
int di_exec_full (const char *path, const char *const argv[], di_io_handler *stdout_handler, di_io_handler *stderr_handler, void *io_user_data, di_handler *prepare_handler, void *prepare_user_data);
int di_exec_shell (const char *const cmd);
int di_exec_shell_full (const char *const cmd, di_io_handler *stdout_handler, di_io_handler *stderr_handler, void *io_user_data, di_handler *prepare_handler, void *prepare_user_data);
int di_exec_shell_log (const char *const cmd);

di_handler di_exec_prepare_chroot;
di_io_handler
  di_exec_io_log;

/** @} */
#endif
