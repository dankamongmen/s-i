/*
 *  string.c -- String handling functions
 *  Copyright (C) 2007  Robert Millan <rmh@aybabtu.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include <nsis/pluginapi.h>

void __declspec (dllexport) bcdedit_extract_id (HWND hwndParent, int string_size, char *variables, stack_t ** stacktop, extra_parameters * extra)
{
  EXDLL_INIT ();

  char msg[1024];
  char *p = msg;

  popstring (msg);

  while (*p)
    {
      if (p[0] == '{' && p[37] == '}')
	{
	  p[38] = '\0';
	  pushstring (p);
	  return;
	}
      p++;
    }

  pushstring ("error");
}
