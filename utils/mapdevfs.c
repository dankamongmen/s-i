/*
 *  map devfs names to old-style names
 *
 */

#include <debian-installer.h>

int main(int argc, char **argv) {
    static char buf[128];
    size_t len = sizeof(buf);

    if (argc != 2) {
        fprintf(stderr, "Wrong number of args: mapdevfs <path>\n");
        return 1;
    }

    if ((len = di_mapdevfs(argv[1], buf, len))) {
       printf("%s\n", buf);
        return 0;
    }

    return 1;
}
