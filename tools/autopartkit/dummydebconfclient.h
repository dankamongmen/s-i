#ifndef _DUMMYDEBCONFCLIENT_H
#define _DUMMYDEBCONFCLIENT_H

#define debconfclient my_debconfclient
#define debconfclient_new my_debconfclient_new
#define debconfclient_delete my_debconfclient_delete

struct debconfclient {
  const char *value;
  void (*command)(struct debconfclient *client, const char *operation, ...);
  void (*commandf)(struct debconfclient *client, const char *operation, ...);
};

struct debconfclient *my_debconfclient_new(void);
void my_debconfclient_delete(struct debconfclient *client);

/* Helper macros for the debconf commands */
#define debconf_input(_client, _priority, _question) \
    _client->commandf(_client, "INPUT %s %s", _priority, _question)

#define debconf_go(_client) \
    _client->command(_client, "GO", NULL)

#define debconf_fset(_client, _question, _flag, _value) \
    _client->commandf(_client, "FSET %s %s %s", _question, _flag, _value)

#define debconf_get(_client, _question) \
    _client->commandf(_client, "GET %s", _question)

#define debconf_subst(_client, _question, _var, _value) \
    _client->commandf(_client, "SUBST %s %s %s", _question, _var, _value)

#define debconf_settitle(_client, _title) \
    _client->commandf(_client, "SETTITLE %s", _title)

#endif /* _DUMMYDEBCONFCLIENT_H */
