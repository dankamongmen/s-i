/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: priority.c
 *
 * Description: routines to handle priority field parsing and comparison
 *
 * $Id: priority.c,v 1.5 2001/01/20 02:36:34 tausq Rel $
 *
 * cdebconf is (c) 2000-2001 Randolph Chung and others under the following
 * license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ***********************************************************************/
#include "common.h"
#include "priority.h"
#include <string.h>

/*
 * Function: priority_code
 * Input: const char *p - priority string
 * Output: int - integer value representing priority
 * Description: returns an integer value suitable for comparison priorities
 * Assumptions: none
 */
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

/*
 * Function: priority_compare
 * Input: const char *p1, const char *p2 - priorities to compare
 * Output: int - <0 if p1<p2, ==0 if p1==p2, >0 if p1>p2
 * Description: compares two priorities
 * Assumptions: none
 */
int priority_compare(const char *p1, const char *p2)
{
	int i1, i2;

	i1 = priority_code(p1);
	i2 = priority_code(p2);

	INFO(INFO_VERBOSE, "Comparing priorities %s (%d) with %s (%d)\n",
		p1, i1, p2, i2);

	if (i1 > i2)
		return 1;
	else if (i1 == i2)
		return 0;
	else
		return -1;
}
