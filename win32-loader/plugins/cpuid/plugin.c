#include <windows.h>
#include "exdll.h"

void __declspec(dllexport) get_arch (HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra)
{
  EXDLL_INIT();

  setuservariable (INST_0, check_64bit () ? "amd64" : "i386");
}
