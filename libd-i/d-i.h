/* 
   d-i.h - common utilities for debian-installer
*/

#define MAXLINE 512

int di_execlog (const char *incmd);
void di_log(char *msg);
int di_check_dir (const char *dirname);

