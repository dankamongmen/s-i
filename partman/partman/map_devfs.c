#include <debian-installer.h>
#include <stdio.h>



#define MAX_PATH 1000

int
main(int argc, char *argv[])
{
        char device[MAX_PATH];
        if (argc <= 0)
                return 0;
        di_system_devfs_map_from(argv[1], device, MAX_PATH);
        if (device == NULL || strlen(device) == 0)
                return 0;
        puts(device);
        return 0;
}


/*
Local variables:
indent-tabs-mode: nil
c-file-style: "linux"
End:
*/
