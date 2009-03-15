/*
 *  systeminfo.c - return some system information for preseeding
 *  Copyright (C) 2007  Paul Wise <pabs@debian.org>
 *  Copyright (C) 2009  Robert Millan <rmh@aybabtu.com>
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
#include "exdll.h"

char buf[1024];

void computername (COMPUTER_NAME_FORMAT format, HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra)
{
	EXDLL_INIT();
	BOOL result;
	DWORD dwStringSize = g_stringsize;
	stack_t *th;
	if (!g_stacktop) return;
	th = (stack_t*) GlobalAlloc(GPTR, sizeof(stack_t) + g_stringsize);
	result = GetComputerNameExA(format, th->text, &dwStringSize);
	th->next = *g_stacktop;
	*g_stacktop = th;
	wsprintf(buf, "%u", result);
	pushstring(buf);
}

void __declspec(dllexport) hostname (HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra)
{
	computername(ComputerNameDnsHostname,hwndParent,string_size,variables,stacktop,extra);
}

void __declspec(dllexport) domain (HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra)
{
	computername(ComputerNameDnsDomain,hwndParent,string_size,variables,stacktop,extra);
}

void __declspec(dllexport)
username (HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra)
{
  EXDLL_INIT();
  DWORD sz = sizeof(buf);
  if (!GetUserNameA (buf, &sz))
    buf[0] = '\0';
  pushstring (buf);
}

void __declspec(dllexport) keyboard_layout (HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra)
{
	EXDLL_INIT();
	HKL hkl = GetKeyboardLayout(0);
	wsprintf(buf, "%u", hkl);
	pushstring(buf);
}

/* Find the partition used by ntldr or bootmgr to boot, aka the first BIOS drive.  */
void __declspec(dllexport) find_system_partition (HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra)
{
  EXDLL_INIT();
  char dosdevice[3];

  dosdevice[1] = ':';
  dosdevice[2] = '\0';
  for (dosdevice[0] = 'C'; dosdevice[0] <= 'Z'; (dosdevice[0])++)
    {
      if (QueryDosDeviceA (dosdevice, buf, sizeof(buf)) == 0)
	continue;
      if (! strcmp (buf, "\\Device\\HarddiskVolume1"))
	{
	  pushstring (dosdevice);
	  return;
	}
    }
  pushstring ("failed");
}
