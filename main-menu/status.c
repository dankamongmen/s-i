/*
 * dpkg status file reading routines
 */

#include "main-menu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <search.h>
#include <locale.h>

/*
 * Read status file and generate and return a linked list of packages.
 */
struct package_t *status_read(void) {
	FILE *f;
	int i, k;
	struct package_t *found, *plist, *p;

	tree_clear();

	if ((f = fopen(STATUSFILE, "r")) == NULL) {
		perror(STATUSFILE);
		return 0;
	}
        plist = di_pkg_parse(f);
        fclose(f);

        for (p = plist; p != NULL; p = p->next)
        {
            tree_add(p);
        }

        for (p = plist; p != NULL; p = p->next)
        {
            for (k = 0; p->provides[k] != 0; k++)
            {
                found = tree_find(p->provides[k]);
                if (found == NULL)
                {
                    found = (struct package_t *)malloc(sizeof(struct package_t));
                    memset(found, 0, sizeof(struct package_t));
                    found->package = strdup(p->provides[k]);
                    tree_add(found);
                    /* TODO: Do we want the virtual packages in the list
                     * or just the tree? */
                    found->next = plist;
                    plist = found;
                }
                for (i = 0; found->depends[i] != 0; i++)
                    ;
                found->depends[i] = p->package;
                found->depends[i+1] = 0;
            }
        }

	return plist;
}
