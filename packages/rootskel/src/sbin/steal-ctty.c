/*
 * Licensed under GPLv2
 *
 * Adapted for Debian Installer by Jérémy Bobbio <lunar.debian.org> from
 * cttyhack from busybox 1.11, which is
 *
 * Copyright (c) 2007 Denys Vlasenko <vda.linux@googlemail.com>
 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char ** argv)
{
    int fd;

    if (-1 == (fd = open(argv[1], O_RDWR))) {
        perror("steal-ctty");
        return 1;
    }
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    while (fd > 2) {
        close(fd--);
    }
    ioctl(0, TIOCSCTTY, (char *) 1);
    execvp(argv[2], &argv[2]);
    /* never reached. */
    return 0;
}
