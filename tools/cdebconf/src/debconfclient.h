#ifndef _DEBCONFCLIENT_H_
#define _DEBCONFCLIENT_H_

struct debconfclient {
	char *value;

	/* methods */
	/* send a command, arglist should be NULL terminated */
	int (*command)(struct debconfclient *client, const char *cmd, ...);
	char *(*ret)(struct debconfclient *client);
};

struct debconfclient *debconfclient_new(void);
void debconfclient_delete(struct debconfclient *client);

#endif
