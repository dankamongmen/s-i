#include "main-menu.h"

#include <stdio.h>
#include <search.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static void depends_sort_visit(struct package_t **ordered,
		struct package_t *pkgs, struct package_t *pkg) {
	/* Topological sort algorithm:
	 * ordered is the output list, pkgs is the dependency graph, pkg is
	 * the current node
	 *
	 * recursively add all the adjacent nodes to the ordered list,
	 * marking each one as visited along the way
	 *
	 * yes, this algorithm looks a bit odd when all the params have the
	 * same type :-)
	 */
	unsigned short i;
	struct package_t *newnode;

	/* mark node as processing */
	pkg->color = COLOR_GRAY;

	/* visit each not-yet-visited node */
	for (i = 0; i < pkg->requiredcount; i++)
		if (pkg->requiredfor[i]->color == COLOR_WHITE)
			depends_sort_visit(ordered, pkgs, pkg->requiredfor[i]);
	
	/* add it to the list */
	newnode = (struct package_t *)malloc(sizeof(struct package_t));
	/* make a shallow copy */
	*newnode = *pkg;
	newnode->next = *ordered;
	*ordered = newnode;

	/* mark node as done */
	pkg->color = COLOR_BLACK;
}

static struct package_t *depends_sort(struct package_t *pkgs) {
	struct package_t *ordered = NULL;
	struct package_t *pkg;

	for (pkg = pkgs; pkg != 0; pkg = pkg->next)
		pkg->color = COLOR_WHITE;

	for (pkg = pkgs; pkg != 0; pkg = pkg->next)
		if (pkg->color == COLOR_WHITE)
			depends_sort_visit(&ordered, pkgs, pkg);
	
	/* Leaks the old list... return the new one... */
	return ordered;
}

int main (int argc, char **argv) {
	void *status;
	
	status = status_read();
		
	return(0);
}
