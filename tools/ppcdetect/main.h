
#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0


static void detect_newworld();
static void detect_prep();
static void detect_chrp();

int check_value(const char *key, const char* value, const char *msg);
int main(int argc, char *argv[]);

#endif /* __MAIN_H__ */

