#ifndef _STDERR_LOG_H
#define _STDERR_LOG_H

#define STDERR_LOG	"/var/log/stderr.log"

void intercept_stderr(void);
void display_stderr_log(const char *package);

#endif /* _STDERR_LOG_H */
