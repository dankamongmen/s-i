/* 
   debian-installer.h - common utilities for debian-installer
*/

#define MAXLINE 512

int di_prebaseconfig_append (const char *udeb, const char *format, ...);
int di_execlog (const char *incmd);
void di_log(const char *msg);
int di_check_dir (const char *dirname);
int di_snprintfcat (char *str, size_t size, const char *format, ...);
