/*
 * Anna's not nearly apt, but for the Debian installer, it will do.
 *
 * Anna is copyright 2000 by Joey Hess, under the terms of the GPL.
 * Apologetically dedicated to my sister, Anna.
 */

#include "anna.h"

/* 
 * This function takes a linked list of available packages, decides which
 * are worth installing, creates a linked list of those, and returns it.
 * TODO: How it will eventually makes such choices is unknown, for now
 * it just returns all of the packages.
 */
struct package_t *select_packages (struct package_t *packages) {
	return packages;
}

int main (int argc, char **argv) {
	struct package_t *packages=get_packages();
	printf("%s\n", packages->package);
}
