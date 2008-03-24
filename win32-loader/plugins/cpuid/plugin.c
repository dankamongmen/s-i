#include <windows.h>
#include "exdll.h"

HINSTANCE g_hInstance;

HWND g_hwndParent;

void __declspec(dllexport) get_arch (HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra)
{
  g_hwndParent=hwndParent;

  EXDLL_INIT();

  setuservariable (INST_0, check_64bit () ? "amd64" : "i386");
}

BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
  g_hInstance=hInst;
        return TRUE;
}
