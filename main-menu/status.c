#include "main-menu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Read status file into memory as an array, preserving only those fields
 * that are needed. The array is terminated with an empty package_t struct.
 */
struct package_t *status_read(void) {
	FILE *f;
	char *b, buf[BUFSIZE];
	int i, alloced=PACKAGECHUNK;
	struct package_t *packages = malloc(sizeof(struct package_t) * alloced);
	struct package_t *p = packages;

	if ((f = fopen(STATUSFILE, "r")) == NULL) {
		perror(STATUSFILE);
		return 0;
	}

	while (fgets(buf, BUFSIZE, f) && !feof(f)) {
		buf[strlen(buf)-1] = 0;
		if (*buf == 0) {
			p++;
			if (p == packages + alloced) {
printf("realloc at %i\n", p-packages);
				packages=realloc(packages, sizeof(struct package_t) * (alloced + PACKAGECHUNK));
				p = packages + alloced;
				alloced += PACKAGECHUNK;
			}
		}
		else if (strstr(buf, "Package: ") == buf) {
			memset(p, 0, sizeof(struct package_t));
			p->package = strdup(buf+9);
		}
		else if (strstr(buf, "Installer-Menu-Item: ") == buf) {
			p->installer_menu_item=atoi(buf+21);
		}
		else if (strstr(buf, "Description: ") == buf) {
			/* Short description only. */
			p->description = strdup(buf+13);
		}
		else if (strstr(buf, "Depends: ") == buf) {
			/*
			 * Basic depends line parser. Can ignore versioning
			 * info since the depends are already satisfied.
			 */
			b=strdup(buf+9);
			i = 0;
			while (*b != 0 && *b != '\n') {
				if (*b != ' ') {
					if (*b == ',') {
						*b = 0;
						p->depends[++i] = 0;
					}
					else if (p->depends[i] == 0) {
						p->depends[i] = b;
					}
				}
				else {
					*b = 0; /* eat the space... */
				}
				b++;
			}
			*b = 0;
			p->depends[i+1] = 0;
		}
		else if (strstr(buf, "Provides: ") == buf) {
			/*
			 * A provides causes a fake package to be made,
			 * that depends on the package that provides it. If
			 * the fake package already exists, just add the
			 * providing package to its dependancy list. This
			 * means that virtual packages are actually ANDed
			 * for the purposes of this program.
			 */
		}
	}
	fclose(f);
	
	return packages;
}
