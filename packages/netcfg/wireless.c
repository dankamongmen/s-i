/* Wireless support using iwlib for netcfg.
 * (C) 2004 Joshua Kwan, Bastian Blank
 *
 * Licensed under the GNU General Public License
 */

#include "netcfg.h"
#include <iwlib.h>
#include <sys/types.h>
#include <assert.h>

/* Wireless mode */
wifimode_t mode = MANAGED;

/* wireless config */
char* wepkey = NULL;
char* essid = NULL;

/* IW socket for global use - init in main */
int wfd = 0;

int is_wireless_iface (const char* iface)
{
  wireless_config wc;

  if (wfd == 0)
    wfd = iw_sockets_open();
  
  return (iw_get_basic_config (wfd, (char*)iface, &wc) == 0);
}

int netcfg_wireless_set_essid (struct debconfclient * client, char *iface)
{
  int ret, couldnt_associate = 0;
  wireless_config wconf;
  char* tf = NULL, *user_essid = NULL, *ptr = wconf.essid;

  if (wfd == 0) /* shouldn't happen */
    wfd = iw_sockets_open();

  iw_get_basic_config (wfd, iface, &wconf);

  debconf_subst(client, "netcfg/wireless_essid", "iface", iface);
  debconf_subst(client, "netcfg/wireless_adhoc_managed", "iface", iface);

  debconf_input(client, "low", "netcfg/wireless_adhoc_managed");

  if (debconf_go(client) == 30)
    return GO_BACK;

  debconf_get(client, "netcfg/wireless_adhoc_managed");

  if (!strcmp(client->value, "Ad-hoc network (Peer to peer)"))
    mode = ADHOC;

  wconf.has_mode = 1;
  wconf.mode = mode;

  debconf_input(client, "low", "netcfg/wireless_essid");

  if (debconf_go(client) == 30)
    return GO_BACK;

  debconf_get(client, "netcfg/wireless_essid");
  tf = strdup(client->value);

automatic:
  /* question not asked or user doesn't care or we're successfully associated */
  if (!empty_str(wconf.essid) || empty_str(client->value)) 
  {
    int i, success = 0;

    /* Default to any AP */
    wconf.essid[0] = '\0';
    wconf.essid_on = 0;

    iw_set_basic_config (wfd, iface, &wconf);

    /* Wait for association.. (MAX_SECS seconds)*/
#define MAX_SECS 3

    debconf_progress_start(client, 0, MAX_SECS, "netcfg/wifi_progress_title");
    debconf_progress_info(client, "netcfg/wifi_progress_info");
    netcfg_progress_displayed = 1;

    for (i = 0; i <= MAX_SECS; i++)
    {
      ifconfig_up(iface);
      sleep (1);
      iw_get_basic_config (wfd, iface, &wconf);

      if (!empty_str(wconf.essid))
      {
	/* Save for later */
	debconf_set(client, "netcfg/wireless_essid", wconf.essid);
	debconf_progress_set(client, MAX_SECS);
	success = 1;
	break;
      }

      debconf_progress_step(client, 1);
      ifconfig_down(iface);
    }

    debconf_progress_stop(client);
    netcfg_progress_displayed = 0;

    if (success)
      return 0;

    couldnt_associate = 1;
  }
  /* yes, wants to set an essid by himself */

  if (strlen(tf) <= IW_ESSID_MAX_SIZE) /* looks ok, let's use it */
    user_essid = tf;

  while (!user_essid || empty_str(user_essid) ||
      strlen(user_essid) > IW_ESSID_MAX_SIZE)
  {
    /* Misnomer of a check. Basically, if we went through autodetection,
     * we want to enter this loop, but we want to suppress anything that
     * relied on the checking of tf/user_essid (i.e. "", in most cases.) */
    if (!couldnt_associate)
    {
      debconf_subst(client, "netcfg/invalid_essid", "essid", user_essid);
      debconf_input(client, "high", "netcfg/invalid_essid");
      debconf_go(client);
    }

    ret = debconf_input(client,
	couldnt_associate ? "critical" : "low",
	"netcfg/wireless_essid");
   
    /* But now we'd not like to suppress any MORE errors */
    couldnt_associate = 0;
    
    /* we asked the question once, why can't we ask it again? */
    assert (ret != 30);

    if (debconf_go(client) == 30) /* well, we did, but he wants to go back */
      return GO_BACK;

    debconf_get(client, "netcfg/wireless_essid");

    if (empty_str(client->value))
      goto automatic;

    free(user_essid);
    user_essid = strdup(client->value);
  }

  essid = user_essid;

  memset(ptr, 0, IW_ESSID_MAX_SIZE + 1);
  snprintf(wconf.essid, IW_ESSID_MAX_SIZE + 1, "%s", essid);
  wconf.has_essid = 1;
  wconf.essid_on = 1;

  iw_set_basic_config (wfd, iface, &wconf);

  return 0;
}

void unset_wep_key (char* iface)
{
  wireless_config wconf;
  int ret;

  if (!wfd)
    wfd = iw_sockets_open();

  iw_get_basic_config(wfd, iface, &wconf);

  wconf.has_key = 1;
  wconf.key[0] = '\0';
  wconf.key_flags = IW_ENCODE_DISABLED | IW_ENCODE_NOKEY;
  wconf.key_size = 0;

  ret = iw_set_basic_config (wfd, iface, &wconf);
}

int netcfg_wireless_set_wep (struct debconfclient * client, char* iface)
{
  wireless_config wconf;
  char* rv = NULL;
  int ret, keylen;
  unsigned char buf [IW_ENCODING_TOKEN_MAX];
  
  iw_get_basic_config (wfd, iface, &wconf);

  debconf_subst(client, "netcfg/wireless_wep", "iface", iface);
  ret = my_debconf_input (client, "high", "netcfg/wireless_wep", &rv);

  if (ret == 30)
    return GO_BACK;

  if (empty_str(rv))
  {
    unset_wep_key (iface);

    if (wepkey != NULL)
    {
      free(wepkey);
      wepkey = NULL;
    }
    
    return 0;
  }

  while ((keylen = iw_in_key (rv, buf)) == -1)
  {
    debconf_subst(client, "netcfg/invalid_wep", "wepkey", rv);
    debconf_input(client, "high", "netcfg/invalid_wep");
    debconf_go(client);
    
    ret = my_debconf_input (client, "high", "netcfg/wireless_wep", &rv);
  }

  wconf.has_key = 1;
  wconf.key_size = keylen;
  wconf.key_flags = IW_ENCODE_ENABLED | IW_ENCODE_OPEN;
  
  strncpy (wconf.key, buf, keylen);

  wepkey = strdup(rv);

  iw_set_basic_config (wfd, iface, &wconf);

  return 0;
}
