/*
 * small helper utility to grep for a channel type from the chantype bitfiled
 *
 * written by Frank Kirschner
 * maintained by the boot-floppies team, debian-boot@lists.debian.org
 * copyright (c) 2001
 * This is free software under the GNU General Public License.
 */

#include <stdio.h>
#include <stdlib.h>

/* example output:
 * 0x0000 0x0840 0x04  0x3088 0x60  0x0000 0x00 0x80 [...}
 */

#define MAX_STRING_LENGTH 1024

int main(int argc, char **argv)
{
    int a, bits;
    char str[MAX_STRING_LENGTH];

    if (argc != 2)
    {
        fprintf(stdout, "usage: changrep <bitfield>\n");
        exit(1);
    }

    sscanf(argv[1],"%i",&a);

    do
    {
        if (fgets(str,MAX_STRING_LENGTH,stdin) != NULL)
        {
            if (sscanf(str,"%*i %*i %i %*i",&bits) == 1)
            {
                if (bits & a)
                {
                    fprintf(stdout, str);
                }
            }
            else
            {
                fprintf(stderr, "error parsing '%s'\n",str);
            }
        }
    }
    while (!feof(stdin));
    exit(0);
}

