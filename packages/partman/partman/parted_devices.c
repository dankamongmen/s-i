#include <parted/parted.h>

int
main(int argc, char *argv[])
{
        PedDevice *dev;
        ped_exception_fetch_all();
        ped_device_probe_all();
        for (dev = NULL; NULL != (dev = ped_device_get_next(dev));) {
		if (dev->read_only)
			continue;
                printf("%s\t%lli\t%s\n",
                       dev->path,
		       dev->length * PED_SECTOR_SIZE,
                       dev->model);
        }
        return 0;
}

/*
Local variables:
indent-tabs-mode: nil
c-file-style: "linux"
c-font-lock-extra-types: ("Ped\\sw+")
End:
*/
