/**
 * @file debconfclient.h
 * @brief debconf client support interface
 *
 * \defgroup debconf client object
 * @{
 */
#ifndef _DEBCONFCLIENT_H_
#define _DEBCONFCLIENT_H_

/**
 * @brief debconf client object
 */
struct debconfclient {
    /** internal use only */
	char *value;

	/* methods */
    /**
     * @brief sends a command to the confmodule
     * @param struct debconfclient *client - client object
     * @param const char *command, ... - null terminated command string
     * @return return code from confmodule
     */
	int (*command)(struct debconfclient *client, const char *cmd, ...);

    /**
     * @brief simple accessor method for the return value
     * @param struct debconfclient *client - client object
     * @return char * - return value
     */
	char *(*ret)(struct debconfclient *client);
};

/**
 * @brief creates a debconfclient object
 * @return struct debconfclient * - newly created debconfclient object
 */
struct debconfclient *debconfclient_new(void);

/**
 * @brief destroys the debconfclient object 
 * @param struct debconfclient *client - client object to destroy
 */
void debconfclient_delete(struct debconfclient *client);

/**
 * @}
 */

#endif
