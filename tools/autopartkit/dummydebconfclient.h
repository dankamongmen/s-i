#define debconfclient my_debconfclient
#define debconfclient_new my_debconfclient_new
#define debconfclient_delete my_debconfclient_delete

struct debconfclient {
  const char *value;
  void (*command)(struct debconfclient *client, const char *operation, ...);
};

struct debconfclient *debconfclient_new(void);
void debconfclient_delete(struct debconfclient *client);
