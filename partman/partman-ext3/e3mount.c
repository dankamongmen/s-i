#include <sys/mount.h>

int
main(int argc, char *argv[])
{
        int ret;
        ret = mount(argv[1], argv[2], "ext3", MS_MGC_VAL, 0);
        return ret != 0;
}

/*
Local variables:
indent-tabs-mode: nil
c-file-style: "linux"
c-font-lock-extra-types: ("FILE" "\\sw+_t" "bool")
End:
*/
