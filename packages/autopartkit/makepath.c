#include <string.h>
#include <alloca.h>
#include <sys/stat.h>
#include <sys/types.h>

int
make_path(const char *pathname, mode_t mode)
{
    char *dirpath;
    char *slash;

    dirpath = alloca(strlen(pathname) + 1);
    strcpy(dirpath, pathname);

    slash = dirpath;

    while ('/' == *slash)
        ++slash;

    while (1)
    {
	slash = strchr(slash, '/');
	if (slash)
	    *slash = '\0';

	if (0 != mkdir(dirpath, mode))
	{
	    struct stat statbuf;
	    if (stat(dirpath, &statbuf))
	        return -1;
	    if (!S_ISDIR(statbuf.st_mode))
	        return -1;
	}

	if (slash)
	{
	    *slash = '/';
	    slash++;
	}
	else
	    break;
    }
    return 0;
}
