/*
 *  map devfs names to old-style names
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

struct diskentry {
    int major;
    int minor;
    char *name; /* hda, hdb, no leading /dev */
};

/* taken from devices.txt in the kernel source, order is important,
 large before smaller. */

struct diskentry diskentries[] = {
    {91,  64, "hdt"},
    {91,   0, "hds"},
    {90,  64, "hdr"},
    {90,   0, "hdq"},
    {89,  64, "hdp"},
    {89,   0, "hdo"},
    {88,  64, "hdn"},
    {88,   0, "hdm"},
    {65, 240, "sdaf"},
    {65, 224, "sdae"},
    {65, 208, "sdad"},
    {65, 192, "sdac"},
    {65, 176, "sdab"},
    {65, 160, "sdaa"},
    {65, 176, "sdz"},
    {65, 160, "sdy"},
    {65, 144, "sdx"},
    {65, 112, "sdw"},
    {65,  96, "sdv"},
    {65,  80, "sdu"},
    {65,  64, "sdt"},
    {65,  32, "sds"},
    {65,  16, "sdr"},
    {65,   0, "sdq"},
    {57,  64, "hdl"},
    {57,   0, "hdk"},
    {56,  64, "hdj"},
    {56,   0, "hdi"},
    {34,  64, "hdh"},
    {34,   0, "hdg"},
    {33,  64, "hdf"},
    {33,   0, "hde"},
    {22,  64, "hdd"},
    {22,   0, "hdc"},
    { 8, 240, "sdp"},
    { 8, 224, "sdo"},
    { 8, 208, "sdn"},
    { 8, 192, "sdm"},
    { 8, 176, "sdl"},
    { 8, 160, "sdk"},
    { 8, 176, "sdj"},
    { 8, 160, "sdi"},
    { 8, 144, "sdh"},
    { 8, 112, "sdg"},
    { 8,  96, "sdf"},
    { 8,  80, "sde"},
    { 8,  64, "sdd"},
    { 8,  32, "sdc"},
    { 8,  16, "sdb"},
    { 8,   0, "sda"},
    { 3,  64, "hdb"},
    { 3,   0, "hda"},
    { 0,   0,  NULL} };

/* Returns the devfs path name normalized into a "normal" (hdaX, sdaX)
 * name.  The return value is malloced, which means that the caller
 * needs to free it.  This is kinda hairy, but I don't see any other
 * way of operating with non-devfs systems when we use devfs on the
 * boot medium.  The numbers are taken from devices.txt in the
 * Documentation subdirectory off the kernel sources.
 */

char *normalize_devfs(const char* path)
{
    int ret;
    struct stat statbuf;
    struct diskentry *de;

    char *retval = NULL;

    int partnum;

    if (0 == strcmp("none", path))
	return strdup(path);

    ret = stat(path,&statbuf);
    if (ret == -1)
    {
        return NULL;
    }
    
    de = diskentries;
    while (de->name != NULL) {
        if (major(statbuf.st_rdev) == de->major && 
            minor(statbuf.st_rdev) >= de->minor) {
            break;
        }
        de++;
    }
    partnum = minor(statbuf.st_rdev) - de->minor;

    if (0 == partnum)
        asprintf(&retval, "/dev/%s", de->name);
    else
        asprintf(&retval, "/dev/%s%d", de->name, partnum);

    return retval;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Wrong number of args: mapdevfs <path>\n");
        return 1;
    }
    printf("%s\n", normalize_devfs(argv[1]));
    return 0;
}
