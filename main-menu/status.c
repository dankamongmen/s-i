#include "main-menu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <search.h>

/*
 * Read status file and generate and return a linked list of packages.
 */
struct package_t *status_read(void) {
	FILE *f;
	char *b, buf[BUFSIZE];
	int i;
	struct package_t virt, *newp, *p = 0;
	void *package_tree = NULL;
	void *found;

	if ((f = fopen(STATUSFILE, "r")) == NULL) {
		perror(STATUSFILE);
		return 0;
	}

	while (fgets(buf, BUFSIZE, f) && !feof(f)) {
		buf[strlen(buf)-1] = 0;
		if (strstr(buf, "Package: ") == buf) {
			newp = malloc(sizeof(struct package_t));
			memset(newp, 0, sizeof(struct package_t));
			newp->next = p;
			p = newp;
			p->package = strdup(buf+9);
			tsearch(p, &package_tree, package_compare);
		}
		else if (strstr(buf, "Installer-Menu-Item: ") == buf) {
			p->installer_menu_item = atoi(buf+21);
		}
		else if (strstr(buf, "Status: ") == buf) {
			if (strstr(buf, " unpacked")) {
				p->status = STATUS_UNPACKED;
			}
			else if (strstr(buf, " installed")) {
				p->status = STATUS_INSTALLED;
			}
			else {
				p->status = STATUS_UNKNOWN;
			}
		}
		else if (strstr(buf, "Description: ") == buf) {
			/* Short description only. */
			/*
			 * TODO: need to get translated data from
			 * somewhere, if in a different locale.
			 */
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
			virt.package = buf+10;
			if ((found = tfind(&virt, &package_tree, package_compare))) {
				newp=*(struct package_t **)found;
			}
			else {
				newp = malloc(sizeof(struct package_t));
				memset(newp, 0, sizeof(struct package_t));
				newp->next=p->next;
				p->next=newp;
				newp->package = strdup(virt.package);
				tsearch(newp, &package_tree, package_compare);
			}
			for (i=0; newp->depends[i] != 0; i++);
			newp->depends[i] = p->package;
			newp->depends[i+1] = 0;
		}
	}
	fclose(f);
	
	return p;
}
