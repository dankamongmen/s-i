#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cdebconf/debconfclient.h>
#include <debian-installer.h>

#include <libsysfs.h>

#define TEMPLATE_PREFIX	"s390-netdevice/"

static struct debconfclient *client;

enum channel_type
{
	CHANNEL_TYPE_CU3088_ALL,
	CHANNEL_TYPE_CU3088_CTC,
	CHANNEL_TYPE_QETH,
};

struct channel
{
	int key;
	char name[SYSFS_NAME_LEN];
	enum channel_type type;
	unsigned int cu_type;
	unsigned int cu_model;
};

enum device_type
{
	DEVICE_TYPE_CTC,
	DEVICE_TYPE_QETH,
	DEVICE_TYPE_IUCV,
};

struct device
{
	int key;
	enum device_type type;
	union
	{
		struct
		{
			struct channel *channels[2];
			int protocol;
		} ctc;
		struct
		{
			struct channel *channels[3];
			int port;
			char portname[32];
		} qeth;
		struct
		{
			char peername[32];
		} iucv;
	};
};

struct device *device_current;

static di_tree *channels;
static di_tree *devices;

struct driver
{
	const char *name;
	int type;
};

static const struct driver drivers[] =
{
	{ "cu3088", CHANNEL_TYPE_CU3088_ALL },
	{ "qeth", CHANNEL_TYPE_QETH },
};

enum
{
	TYPE_NONE = 0,
	TYPE_CTC,
	TYPE_IUCV,
	TYPE_QETH,
}
type = TYPE_NONE;

enum state
{
	BACKUP,
	SETUP,
	DETECT_CHANNELS,
	DETECT_DEVICES,
	GET_NETWORKTYPE,
	GET_CTC_DEVICE,
	GET_CTC_CHANNELS,
	GET_CTC_PROTOCOL,
	GET_QETH_DEVICE,
	GET_QETH_PORT,
	GET_QETH_PORTNAME,
	GET_IUCV_DEVICE,
	GET_IUCV_PEER,
	CONFIRM_CTC,
	CONFIRM_QETH,
	CONFIRM_IUCV,
	WRITE_CTC,
	WRITE_QETH,
	WRITE_IUCV,
	ERROR,
	FINISH
};

enum state_wanted
{
	WANT_NONE = 0,
	WANT_BACKUP,
	WANT_NEXT,
	WANT_FINISH,
	WANT_ERROR
};

static int my_debconf_input (const char *priority, const char *template, char **p)
{
	int ret;

	debconf_fset(client, template, "seen", "false");
	debconf_input(client, priority, template);
	ret = debconf_go(client);
	debconf_get(client, template);
	*p = client->value;
	return ret;
}

static di_compare_func channel_compare;
int channel_compare (const void *key1, const void *key2)
{
	const unsigned int *k1 = key1, *k2 = key2;
	return *k1 - *k2;
}

static int channel_device (const char *i)
{
	unsigned int ret;
	if (sscanf (i, "0.0.%04x", &ret) == 1)
		return ret;
	if (sscanf (i, "%04x", &ret) == 1)
		return ret;
	return -1;
}

static enum state_wanted setup ()
{
	channels = di_tree_new (channel_compare);
	devices = di_tree_new (channel_compare);

	return WANT_NEXT;
}

static enum state_wanted detect_channels_driver (struct dlist *devices, int type)
{
	struct sysfs_device *device;

	dlist_for_each_data (devices, device, struct sysfs_device)
	{
		struct sysfs_attribute *attr_cutype;
		struct channel *current;
		char buf[SYSFS_PATH_MAX];

		/* Ignore already used channels. */
		strncpy (buf, device->path, SYSFS_PATH_MAX);
		strncat (buf, "/group_device", SYSFS_PATH_MAX);
		if (!sysfs_path_is_link (buf))
			continue;

		current = di_new0 (struct channel, 1);
		if (!current)
			return WANT_ERROR;
		current->type = type;

		strncpy (current->name, device->name, sizeof (current->name));
		current->key = channel_device(device->name);

		attr_cutype = sysfs_get_device_attr (device, "cutype");
		sysfs_read_attribute (attr_cutype);
		sscanf (attr_cutype->value, "%04x/%02x", &current->cu_type, &current->cu_model);

		di_tree_insert (channels, current, current);
	}

	return WANT_NONE;
}

static enum state_wanted detect_channels (void)
{
	enum state_wanted ret = WANT_NONE;
	unsigned int i;

	for (i = 0; i < sizeof (drivers) / sizeof (*drivers); i++)
	{
		struct sysfs_driver *driver = sysfs_open_driver ("ccw", drivers[i].name);
		if (driver)
		{
			struct dlist *devices;
			devices = sysfs_get_driver_devices (driver);
			if (devices)
				ret = detect_channels_driver (devices, drivers[i].type);
			sysfs_close_driver (driver);
			if (ret)
				return ret;
		}
	}
	return WANT_NEXT;
}

struct detect_devices_info
{
	struct device *current_device;
};

static di_hfunc detect_devices_each;
static void detect_devices_each (void *key __attribute__ ((unused)), void *value, void *user_data)
{
	struct channel *chan = value;
	struct detect_devices_info *info = user_data;

	switch (chan->type)
	{
		case CHANNEL_TYPE_CU3088_ALL:
			switch (chan->cu_model)
			{
				case 0x8:
					chan->type = CHANNEL_TYPE_CU3088_CTC;
					break;
				default:
					break;
			};
			break;
		case CHANNEL_TYPE_QETH:
			if (!info->current_device)
				info->current_device = di_new0 (struct device, 1);

			if (info->current_device->qeth.channels[1])
			{
				if (info->current_device->qeth.channels[1]->key + 1 == chan->key)
				{
					info->current_device->qeth.channels[2] = chan;
					di_tree_insert (devices, info->current_device, info->current_device);
					info->current_device = NULL;
				}
				else
					info->current_device->type = 0;
			}
			else if (info->current_device->qeth.channels[0])
			{
				if (info->current_device->qeth.channels[0]->key + 1 == chan->key)
					info->current_device->qeth.channels[1] = chan;
				else
					info->current_device->type = 0;
			}

			if (info->current_device && !info->current_device->type)
			{
				info->current_device->type = DEVICE_TYPE_QETH;
				info->current_device->key = chan->key;
				info->current_device->qeth.channels[0] = chan;
			}
			break;
		default:
			break;
	};
}

static enum state_wanted detect_devices (void)
{
	struct detect_devices_info info = { 0, };
	di_tree_foreach (channels, detect_devices_each, &info);
	return WANT_NEXT;
}

static enum state_wanted get_networktype (void)
{
	char *ptr;
	int ret = my_debconf_input ("critical", TEMPLATE_PREFIX "choose_networktype", &ptr);

	if (ret == 30)
		return WANT_BACKUP;
	if (ret)
		return WANT_ERROR;

	if (!strncmp (ptr, "qeth", 4))
		type = TYPE_QETH;
	else if (!strncmp (ptr, "ctc", 3))
		type = TYPE_CTC;
	else if (!strncmp (ptr, "iucv", 4))
		type = TYPE_IUCV;
	else
		return WANT_ERROR;

	return WANT_NEXT;
}

static di_hfunc get_ctc_channels_append;
static void get_ctc_channels_append (void *key __attribute__ ((unused)), void *value, void *user_data)
{
	struct channel *channel = value;
	char *buf = user_data;
	if (channel->type == CHANNEL_TYPE_CU3088_CTC)
		di_snprintfcat (buf, 512, "%s, ", channel->name);
}

static enum state_wanted get_ctc_channels (void)
{
	char buf[64 * 8] = { 0 }, *ptr;
	const char *template;
	int dev, ret;

	di_tree_foreach (channels, get_ctc_channels_append, buf);

	if (!strlen (buf))
	{
		my_debconf_input ("critical", TEMPLATE_PREFIX "ctc/no", &ptr);
		return WANT_BACKUP;
	}

	template = TEMPLATE_PREFIX "ctc/choose_read";
	debconf_subst (client, template, "choices", buf);
	debconf_fset (client, template, "seen", "false");
	debconf_input (client, "critical", template);
	ret = debconf_go (client);
	if (ret == 30)
		return WANT_BACKUP;
	if (ret)
		return WANT_ERROR;
	debconf_get (client, template);

	dev = channel_device (client->value);
	device_current->ctc.channels[0] = di_tree_lookup (channels, &dev);

	template = TEMPLATE_PREFIX "ctc/choose_write";
	debconf_subst (client, template, "choices", buf);
	debconf_fset (client, template, "seen", "false");
	debconf_input (client, "critical", template);
	ret = debconf_go (client);
	if (ret == 30)
		return WANT_BACKUP;
	if (ret)
		return WANT_ERROR;
	debconf_get (client, template);

	dev = channel_device (client->value);
	device_current->ctc.channels[1] = di_tree_lookup (channels, &dev);

	return WANT_NEXT;
}

static enum state_wanted get_ctc_device_iucv_device (enum state state)
{
	device_current = di_new0 (struct device, 1);

	switch (state)
	{
		case GET_CTC_DEVICE:
			device_current->type = DEVICE_TYPE_CTC;
			break;
		case GET_IUCV_DEVICE:
			device_current->type = DEVICE_TYPE_IUCV;
			break;
		default:
			return WANT_ERROR;
	}

	return WANT_NEXT;
}

static enum state_wanted get_ctc_protocol (void)
{
	char *ptr;
	int ret = my_debconf_input ("critical", TEMPLATE_PREFIX "ctc/protocol", &ptr);

	if (ret == 30)
		return WANT_BACKUP;
	if (ret)
		return WANT_ERROR;

	device_current->ctc.protocol = 0;
	if (!strcmp (ptr, "Linux (1)"))
		device_current->ctc.protocol = 1;
	else if (!strcmp (ptr, "OS/390 (3)"))
		device_current->ctc.protocol = 3;

	return WANT_NEXT;
}

static di_hfunc get_qeth_device_append;
static void get_qeth_device_append (void *key __attribute__ ((unused)), void *value, void *user_data)
{
	struct device *device = value;
	char *buf = user_data;
	if (device->type == DEVICE_TYPE_QETH)
		di_snprintfcat (buf, 64 * 28, "%s-%s-%s, ", device->qeth.channels[0]->name, device->qeth.channels[1]->name, device->qeth.channels[2]->name);
}

static enum state_wanted get_qeth_device (void)
{
	char buf[64 * 28] = { 0 }, *ptr;
	const char *template;
	int dev, ret;

	di_tree_foreach (devices, get_qeth_device_append, buf);

	if (!strlen (buf))
	{
		my_debconf_input ("critical", TEMPLATE_PREFIX "qeth/no", &ptr);
		return WANT_BACKUP;
	}

	template = TEMPLATE_PREFIX "qeth/choose";
	debconf_subst (client, template, "choices", buf);
	debconf_fset (client, template, "seen", "false");
	debconf_input (client, "critical", template);
	ret = debconf_go (client);
	if (ret == 30)
		return WANT_BACKUP;
	if (ret)
		return WANT_ERROR;
	debconf_get (client, template);

	dev = channel_device (client->value);
	device_current = di_tree_lookup (devices, &dev);

	return WANT_NEXT;
}

static enum state_wanted get_qeth_port (void)
{
	char *ptr;
	int ret = my_debconf_input ("critical", TEMPLATE_PREFIX "qeth/port", &ptr);

	if (ret == 30)
		return WANT_BACKUP;
	if (ret)
		return WANT_ERROR;

	sscanf (ptr, "%d", &device_current->qeth.port);

	return WANT_NEXT;
}

static enum state_wanted get_qeth_portname_iucv_peer (enum state state)
{
	const char *template = NULL;
	char *ptr, *tmp;
	int ret, i, j;

	switch (state)
	{
		case GET_QETH_PORTNAME:
			template = TEMPLATE_PREFIX "qeth/portname";
			tmp = device_current->qeth.portname;
			break;
		case GET_IUCV_PEER:
			template = TEMPLATE_PREFIX "iucv/peer";
			tmp = device_current->iucv.peername;
			break;
		default:
			return WANT_ERROR;
	}

	ret = my_debconf_input ("critical", template, &ptr);
	if (ret)
		return ret;

	*tmp = '0';

	j = strlen (ptr);
	if (j)
	{
		strcpy (tmp, ptr);
		for (i = 0; i < j; i++)
			tmp[i] = toupper (tmp[i]);
	}

	return WANT_NEXT;
}

static enum state_wanted confirm_ctc (void)
{
	const char *template = TEMPLATE_PREFIX "ctc/confirm";
	int ret;
	char *ptr;

	debconf_subst (client, template, "device_read", device_current->ctc.channels[0]->name);
	debconf_subst (client, template, "device_write", device_current->ctc.channels[1]->name);

	switch (device_current->ctc.protocol)
	{
		case 0:
			ptr = "S/390";
			break;
		case 1:
			ptr = "Linux";
			break;
		case 3:
			ptr = "OS/390";
			break;
		default:
			ptr = "unknown";
	}
	debconf_subst (client,  template, "protocol", ptr);

	debconf_set (client, template, "true");
	ret = my_debconf_input ("medium", template, &ptr);

	if (ret == 30 && !strstr (ptr, "true"))
		return WANT_BACKUP;
	if (ret)
		return WANT_ERROR;

	return WANT_NEXT;
}

static enum state_wanted confirm_qeth (void)
{
	const char *template = TEMPLATE_PREFIX "qeth/confirm";
	int ret;
	char *ptr;

	debconf_subst (client, template, "device0", device_current->ctc.channels[0]->name);
	debconf_subst (client, template, "device1", device_current->ctc.channels[1]->name);
	debconf_subst (client, template, "device2", device_current->ctc.channels[2]->name);

	debconf_set (client, template, "true");
	ret = my_debconf_input ("medium", template, &ptr);

	if (ret == 30 && !strstr (ptr, "true"))
		return WANT_BACKUP;
	if (ret)
		return WANT_ERROR;

	return WANT_NEXT;
}

static enum state_wanted confirm_iucv (void)
{
	return WANT_ERROR;
}

static enum state_wanted write_ctc (void)
{
	return WANT_ERROR;
}

static enum state_wanted write_qeth (void)
{
	return WANT_ERROR;
}

static enum state_wanted write_iucv (void)
{
	return WANT_ERROR;
}

#if 0
static enum state_wanted confirm (void)
{
	const char *template;
	char buf[10], *ptr;
	int ret;

	switch (type)
	{
		case TYPE_QETH:
			template = TEMPLATE_PREFIX "qeth/confirm";
			break;
		case TYPE_CTC:
			template = TEMPLATE_PREFIX "ctc/confirm";
			break;
		case TYPE_LCS:
			template = TEMPLATE_PREFIX "lcs/confirm";
			break;
		case TYPE_IUCV:
			template = TEMPLATE_PREFIX "iucv/confirm";
			break;
		default:
			return -1;
	}

	if (device_qeth_portname_iucv_peer)
		ptr = device_qeth_portname_iucv_peer;
	else
		ptr = "-";
		
	switch (type)
	{
		case TYPE_QETH:
			snprintf (buf, sizeof (buf), "0x%x", devices[device_selected].device_data);
			debconf_subst (client,  template, "device_data", buf);
			debconf_subst (client,  template, "portname", ptr);

		case TYPE_LCS:
			snprintf (buf, sizeof (buf), "%d", device_qeth_lcs_port);
			debconf_subst (client, template, "port", buf);

		case TYPE_CTC:
			snprintf (buf, sizeof (buf), "0x%x", devices[device_selected].device_read);
			debconf_subst (client,  template, "device_read", buf);
			snprintf (buf, sizeof (buf), "0x%x", devices[device_selected].device_write);
			debconf_subst (client,  template, "device_write", buf);
			break;

		case TYPE_IUCV:
			debconf_subst (client,  template, "peer", ptr);
			break;
	}

	switch (type)
	{
		case TYPE_CTC:
			switch (device_ctc_protocol)
			{
				case 0:
					ptr = "S/390";
					break;
				case 1:
					ptr = "Linux";
					break;
				case 3:
					ptr = "OS/390";
					break;
			}
			debconf_subst (client,  template, "protocol", ptr);
			break;
		default:
			break;
	}

	debconf_set (client, template, "true");
	ret = my_debconf_input ("medium", template, &ptr);

	if (ret)
		return ret;
	if (!strstr (ptr, "true"))
		return 1;

	switch (type)
	{
		case TYPE_QETH:
			snprintf (chandev_module_parm, sizeof (chandev_module_parm), "qeth-1,0x%x,0x%x,0x%x,0,%d",
				  devices[device_selected].device_read,
				  devices[device_selected].device_write,
				  devices[device_selected].device_data,
				  device_qeth_lcs_port);
			if (device_qeth_portname_iucv_peer)
				snprintf (chandev_parm, sizeof (chandev_parm), "add_parms,0x%x,0x%x,0x%x,portname:%s",
					  chantype_qeth,
					  devices[device_selected].device_read,
					  devices[device_selected].device_data,
					  device_qeth_portname_iucv_peer);
			break;
		case TYPE_CTC:
			snprintf (chandev_module_parm, sizeof (chandev_module_parm), "ctc-1,0x%x,0x%x,0,%d",
				  devices[device_selected].device_read,
				  devices[device_selected].device_write,
				  device_ctc_protocol);
			break;
		case TYPE_LCS:
			break;
		case TYPE_IUCV:
			break;
	}

	return 0;
	return WANT_ERROR;
}
#endif

#if 0
static enum state_wanted setup (void)
{
	FILE *f;
	char buf[256], buf1[64] = "";

	switch (type)
	{
		case TYPE_QETH:
		case TYPE_CTC:
		case TYPE_LCS:
			f = fopen ("/proc/chandev", "a");
			if (!f)
				return 1;

			if (strlen (chandev_parm))
			{
				snprintf (buf, sizeof (buf), "register-module -t chandev %s %s", module, chandev_parm);
				di_exec_shell_log (buf);
				fprintf (f, "%s\n", chandev_parm);
			}

			snprintf (buf, sizeof (buf), "register-module -t chandev %s %s", module, chandev_module_parm);
			di_exec_shell_log (buf);

			fprintf (f, "%s\n", chandev_module_parm);
			fprintf (f, "noauto\n");
			fprintf (f, "reprobe\n");
			fclose (f);
			break;

		case TYPE_IUCV:
			snprintf (buf1, sizeof (buf1), "iucv=%s", device_qeth_portname_iucv_peer);
			break;
	}

	snprintf (buf, sizeof (buf), "register-module %s %s", module, buf1);
	di_exec_shell_log (buf);

	snprintf (buf, sizeof (buf), "modprobe %s %s", module, buf1);
	di_exec_shell_log (buf);

	return 0;
	return WANT_ERROR;
}
#endif


int main (int argc __attribute__ ((unused)), char *argv[] __attribute__ ((unused)))
{
	di_system_init ("s390-netdevice");

	client = debconfclient_new ();
	debconf_capb (client, "backup");

	enum state state = SETUP;

	while (1)
	{
		enum state_wanted state_want = WANT_ERROR;

		switch (state)
		{
			case BACKUP:
				return 10;
			case SETUP:
				state_want = setup ();
				break;
			case DETECT_CHANNELS:
				state_want = detect_channels ();
				break;
			case DETECT_DEVICES:
				state_want = detect_devices ();
				break;
			case GET_NETWORKTYPE:
				state_want = get_networktype ();
				break;
			case GET_CTC_DEVICE:
			case GET_IUCV_DEVICE:
				state_want = get_ctc_device_iucv_device (state);
				break;
			case GET_CTC_CHANNELS:
				state_want = get_ctc_channels ();
				break;
			case GET_CTC_PROTOCOL:
				state_want = get_ctc_protocol ();
				break;
			case GET_QETH_DEVICE:
				state_want = get_qeth_device ();
				break;
			case GET_QETH_PORT:
				state_want = get_qeth_port ();
				break;
			case GET_QETH_PORTNAME:
			case GET_IUCV_PEER:
				state_want = get_qeth_portname_iucv_peer (state);
				break;
			case CONFIRM_CTC:
				state_want = confirm_ctc ();
				break;
			case CONFIRM_QETH:
				state_want = confirm_qeth ();
				break;
			case CONFIRM_IUCV:
				state_want = confirm_iucv ();
				break;
			case WRITE_CTC:
				state_want = write_ctc ();
				break;
			case WRITE_QETH:
				state_want = write_qeth ();
				break;
			case WRITE_IUCV:
				state_want = write_iucv ();
				break;
			case ERROR:
				return 1;
			case FINISH:
				return 0;
		}

		switch (state_want)
		{
			case WANT_NEXT:
				switch (state)
				{
					case SETUP:
						state = DETECT_CHANNELS;
						break;
					case DETECT_CHANNELS:
						state = DETECT_DEVICES;
						break;
					case DETECT_DEVICES:
						state = GET_NETWORKTYPE;
						break;
					case GET_NETWORKTYPE:
						switch (type)
						{
							case TYPE_CTC:
								state = GET_CTC_DEVICE;
								break;
							case TYPE_QETH:
								state = GET_QETH_DEVICE;
								break;
							case TYPE_IUCV:
								state = GET_IUCV_DEVICE;
								break;
							default:
								state = ERROR;
						}
						break;
					case GET_CTC_DEVICE:
						state = GET_CTC_CHANNELS;
						break;
					case GET_CTC_CHANNELS:
						state = GET_CTC_PROTOCOL;
						break;
					case GET_CTC_PROTOCOL:
						state = CONFIRM_CTC;
						break;
					case GET_QETH_DEVICE:
						state = GET_QETH_PORT;
						break;
					case GET_QETH_PORT:
						state = GET_QETH_PORTNAME;
						break;
					case GET_QETH_PORTNAME:
						state = CONFIRM_QETH;
						break;
					case GET_IUCV_DEVICE:
						state = GET_IUCV_PEER;
						break;
					case GET_IUCV_PEER:
						state = CONFIRM_IUCV;
						break;
					case CONFIRM_CTC:
						state = WRITE_CTC;
						break;
					case CONFIRM_QETH:
						state = WRITE_QETH;
						break;
					case CONFIRM_IUCV:
						state = WRITE_IUCV;
						break;
					case WRITE_CTC:
					case WRITE_QETH:
					case WRITE_IUCV:
						state = FINISH;
						break;
					case BACKUP:
					case ERROR:
					case FINISH:
						state = ERROR;
				}
				break;
			case WANT_BACKUP:
				switch (state)
				{
					case GET_NETWORKTYPE:
						state = BACKUP;
						break;
					default:
						state = ERROR;
				}
				break;
			case WANT_FINISH:
				state = FINISH;
				break;
			default:
				state = ERROR;
		}
	}
}

/* vim: noexpandtab sw=8
 */
