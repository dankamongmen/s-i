/*
 * Anna's Not Nearly Apt, but for the Debian installer, it will do. 
 *
 * Anna is Copyright (C) 2000 by Joey Hess, under the terms of the GPL.
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

/*
 * Retrives and unpacks each package in the linked list.
 */
int install_packages (struct package_t *packages) {
	struct package_t *p;
	
	for (p=packages; p; p=p->next) {
		printf("installing: %s\n", p->package);
	}
}

int main (int argc, char **argv) {
	return ! install_packages(select_packages(get_packages()));
}
