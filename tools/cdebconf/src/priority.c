#include "priority.h"
#include <string.h>

static int priority_code(const char *p)
{
	if (p == 0) return -1;
	if (strcmp(p, "low") == 0)
		return 0;
	if (strcmp(p, "medium") == 0)
		return 1;
	if (strcmp(p, "high") == 0)
		return 2;
	if (strcmp(p, "critical") == 0)
		return 3;
	return -1;
}


int priority_compare(const char *p1, const char *p2)
{
	int i1, i2;

	i1 = priority_code(p1);
	i2 = priority_code(p2);

	if (i1 > i2)
		return -1;
	else if (i1 == i2)
		return 0;
	else
		return 1;
}
