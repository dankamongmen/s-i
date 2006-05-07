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
	CHANNEL_TYPE_QETH,
};

struct channel
{
	int key;
	char name[SYSFS_NAME_LEN];
	char devtype[SYSFS_NAME_LEN];
	enum channel_type type;
};

struct device
{
	int key;
	int devices[3];
};

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
enum state_wanted { WANT_NONE = 0, WANT_BACKUP, WANT_NEXT, WANT_FINISH, WANT_ERROR };

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

static enum state_wanted detect_channels_driver (struct sysfs_driver *driver, int type)
{
	struct dlist *devices;
	struct sysfs_device *device;

	devices = sysfs_get_driver_devices (driver);
	if (!devices)
		return WANT_NONE;

	dlist_for_each_data (devices, device, struct sysfs_device)
	{
		struct sysfs_attribute *attr_devtype;
		struct channel *current;
		char buf[SYSFS_PATH_MAX];

		/* Ignore already used channels. */
		strncpy (buf, device->path, SYSFS_PATH_MAX);
		strncat (buf, "/group_device", SYSFS_PATH_MAX);
		if (!sysfs_path_is_link (buf))
			continue;

		current = di_new (struct channel, 1);
		if (!current)
			return WANT_ERROR;
		current->type = type;

		strncpy (current->name, device->name, sizeof (current->name));
		current->key = channel_device(device->name);

		attr_devtype = sysfs_get_device_attr (device, "devtype");
		sysfs_read_attribute (attr_devtype);
		strncpy (current->devtype, attr_devtype->value, sizeof (current->devtype));

		di_tree_insert (channels, current, current);
	}

	return WANT_NONE;
}

static enum state_wanted detect_channels (void)
{
	struct sysfs_driver *driver;
	enum state_wanted ret;
	unsigned int i;

	for (i = 0; i < sizeof (drivers) / sizeof (*drivers); i++)
	{
		driver = sysfs_open_driver ("ccw", drivers[i].name);
		if (driver)
		{
			ret = detect_channels_driver (driver, drivers[i].type);
			sysfs_close_driver (driver);
			if (ret)
				return ret;
		}
	}
	return WANT_NEXT;
}

static di_hfunc detect_devices_each;
static void detect_devices_each (void *key __attribute__ ((unused)), void *value, void *user_data)
{
}

static enum state_wanted detect_devices (void)
{
	di_tree_foreach (channels, detect_devices_each, NULL);
	return WANT_ERROR;
}

static enum state_wanted get_networktype (void)
{
#if 0
	char *ptr;
	int ret = my_debconf_input ("critical", TEMPLATE_PREFIX "choose_networktype", &ptr);

	if (ret)
		return ret;

	if (!strncmp (ptr, "qeth", 4))
	{
		type = TYPE_QETH;
		type_text = "qeth";
		module = type_text;
	}
	else if (!strncmp (ptr, "ctc", 3))
	{
		type = TYPE_CTC;
		type_text = "ctc";
		module = type_text;
	}
	else if (!strncmp (ptr, "lcs", 3))
	{
		type = TYPE_LCS;
		type_text = "lcs";
		module = type_text;
	}
	else if (!strncmp (ptr, "iucv", 4))
	{
		type = TYPE_IUCV;
		type_text = "iucv";
		module = "netiucv";
	}
	else
		return -1;

	return 0;
#endif
	return WANT_ERROR;
}

static enum state_wanted get_channel (void)
{
#if 0
	FILE *f;
	unsigned int i, k;
	int j, ret;
	int chantype, chantype_ctc, chantype_escon, chantype_lcs;
	char buf[256], *ptr, *ptr2;

	channels = di_new (struct channel, 5);
	channels_items = 0;

	f = fopen (file_chandev, "r");

	if (!f)
		return -1;

	while (fgets (buf, sizeof (buf), f) && strncmp (buf, "chan_type", 9));
	// I know its a hack
	ptr = buf + 23;
	while ((ptr2 = strsep (&ptr, ",")))
	{
		sscanf (ptr2, "ctc=%x", &chantype_ctc);
		sscanf (ptr2, "escon=%x", &chantype_escon);
		sscanf (ptr2, "lcs=%x", &chantype_lcs);
		sscanf (ptr2, "qeth=%x", &chantype_qeth);
	}

	while (fgets (buf, sizeof (buf), f) && strncmp (buf, "channels detected", 17));

	fgets (buf, sizeof (buf), f);
	fgets (buf, sizeof (buf), f);
	fgets (buf, sizeof (buf), f);

	while (fgets (buf, sizeof (buf), f))
		if (sscanf (buf, "0x%*x 0x%4x 0x%2x 0x%4x 0x%2x",
					&channels[channels_items].device,
					&channels[channels_items].chantype,
					&channels[channels_items].cutype,
					&channels[channels_items].cumodel) == 4)
		{
			if (channels[channels_items].chantype & chantype_escon)
				channels[channels_items].chantype |= chantype_ctc;

			channels[channels_items].device_first = 0;
			channels_items++;
			if ((channels_items % 5) == 0)
				channels = realloc (channels, (channels_items + 5) * sizeof (struct channel));
		}
	fclose (f);

	qsort (channels, channels_items, sizeof (struct channel), &channel_sort);

	devices = malloc (5 * sizeof (struct device));

	for (i = 0; i < channels_items; i++)
	{
		k = 0;
		for (j = i; j >= 0; j--)
			if (channels[i].device == channels[j].device + k &&
			    channels[i].chantype == channels[j].chantype &&
			    channels[i].cutype == channels[j].cutype &&
			    channels[i].cumodel == channels[j].cumodel)
				k++;
			else
				break;

		if (channels[i].chantype & chantype_qeth)
		{
			if (!(k % 3))
			{
				devices[devices_items].device_read =
					channels[i-2].device_first =
					channels[i-1].device_first =
					channels[i].device_first =
						channels[i].device - 2;
				devices[devices_items].device_write =
					channels[i].device - 1;
				devices[devices_items].device_data =
					channels[i].device;
				devices[devices_items].chantype =
					channels[i].chantype;
				devices_items++;
				items_qeth++;
			}
		}
		else
		{
			if (!(k % 2))
			{
				devices[devices_items].device_read =
					channels[i-1].device_first =
					channels[i].device_first =
						channels[i].device - 1;
				devices[devices_items].device_write =
					channels[i].device;
				devices[devices_items].chantype =
					channels[i].chantype;
				devices_items++;
				if (channels[i].chantype & chantype_ctc)
					items_ctc++;
				if (channels[i].chantype & chantype_escon)
					items_escon++;
				if (channels[i].chantype & chantype_lcs)
					items_lcs++;
			}
		}

		if ((devices_items % 5) == 0)
			devices = realloc (devices, (devices_items + 5) * sizeof (struct device));
	}

	chandev_parm[0] = '\0';
	chandev_module_parm[0] = '\0';

	switch (type)
	{
		case TYPE_QETH:
			chantype = chantype_qeth;
			items = items_qeth;
			break;
		case TYPE_CTC:
			chantype = chantype_ctc;
			items = items_ctc;
			break;
		case TYPE_LCS:
			chantype = chantype_lcs;
			items = items_lcs;
			break;
		default:
			return -1;
	}

	if (!items)
		switch (type)
		{
			case TYPE_QETH:
				my_debconf_input ("critical", TEMPLATE_PREFIX "qeth/no", &ptr);
				break;
			case TYPE_CTC:
				my_debconf_input ("critical", TEMPLATE_PREFIX "ctc/no", &ptr);
				break;
			case TYPE_LCS:
				my_debconf_input ("critical", TEMPLATE_PREFIX "lcs/no", &ptr);
				break;
			default:
				return -1;
		}

	ptr = malloc (items * 16); // "0x0000-0x0000, "
	ptr[0] = '\0';

	for (i = 0; i < devices_items; i++)
		if (devices[i].chantype & chantype)
		{
			if (chantype == chantype_qeth)
				di_snprintfcat (ptr, items * 16, "0x%04x-0x%04x, ", devices[i].device_read, devices[i].device_data);
			else
				di_snprintfcat (ptr, items * 16, "0x%04x-0x%04x, ", devices[i].device_read, devices[i].device_write);
		}
        if (strlen(ptr) > 2)
		ptr[strlen(ptr) - 2] = '\0';

	switch (type)
	{
		case TYPE_QETH:
			ptr2 = TEMPLATE_PREFIX "qeth/choose";
			break;
		case TYPE_CTC:
			ptr2 = TEMPLATE_PREFIX "ctc/choose";
			break;
		case TYPE_LCS:
			ptr2 = TEMPLATE_PREFIX "lcs/choose";
			break;
		default:
			return -1;
	}

	debconf_subst (client, ptr2, "choices", ptr);
	free (ptr);
	debconf_fset (client, ptr2, "seen", "false");
	debconf_input (client, "critical", ptr2);
	ret = debconf_go (client);
	debconf_get (client, ptr2);

	sscanf (client->value, "0x%x", &i);

	for (device_selected = 0; devices[device_selected].device_read != i; device_selected++);

	return ret;
#endif
	return WANT_ERROR;
}

static enum state_wanted get_ctc_protocol (void)
{
#if 0
	char *ptr;
	int ret = my_debconf_input ("critical", TEMPLATE_PREFIX "ctc/protocol", &ptr);
	if (ret)
		return ret;

	device_ctc_protocol = 0;
	if (!strcmp (ptr, "Linux (1)"))
		device_ctc_protocol = 1;
	else if (!strcmp (ptr, "OS/390 (3)"))
		device_ctc_protocol = 3;

	return 0;
#endif
	return WANT_ERROR;
}

static enum state_wanted get_qeth_lcs_port (void)
{
#if 0
	char *ptr;
	int ret = my_debconf_input ("critical", TEMPLATE_PREFIX "qeth_lcs/port", &ptr);
	if (ret)
		return ret;

	sscanf (ptr, "%d", &device_qeth_lcs_port);

	return 0;
#endif
	return WANT_ERROR;
}

static enum state_wanted get_qeth_portname_iucv_peer (void)
{
#if 0
	const char *template = NULL;
	char *ptr;
	int ret, j, k;

        switch (type)
        {
		case TYPE_QETH:
			template = TEMPLATE_PREFIX "qeth/portname";
			break;
		case TYPE_IUCV:
			template = TEMPLATE_PREFIX "iucv/peer";
			break;
		default:
			break;
	}

	ret = my_debconf_input ("critical", template, &ptr);
	if (ret)
		return ret;

	free (device_qeth_portname_iucv_peer);
	device_qeth_portname_iucv_peer = NULL;

	j = strlen (ptr);

	if (j)
	{
		device_qeth_portname_iucv_peer = strdup (ptr);
		for (k = 0; k < j; k++)
			device_qeth_portname_iucv_peer[k] = toupper (device_qeth_portname_iucv_peer[k]);

		return 0;
	}

	return 1;
#endif
	return WANT_ERROR;
}

static enum state_wanted confirm (void)
{
#if 0
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
#endif
	return WANT_ERROR;
}

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


static enum state_wanted error (void)
{
	char *ptr;

	my_debconf_input ("high", TEMPLATE_PREFIX "error", &ptr);

	return WANT_FINISH;
}

int main (int argc __attribute__ ((unused)), char *argv[] __attribute__ ((unused)))
{
	di_system_init ("s390-netdevice");

	client = debconfclient_new ();
	debconf_capb (client, "backup");

	enum
	{
		BACKUP,
		SETUP,
		DETECT_CHANNELS,
		DETECT_DEVICES,
		GET_NETWORKTYPE,
		GET_CHANNEL,
		GET_CTC_PROTOCOL,
		GET_QETH_LCS_PORT,
		GET_QETH_PORTNAME_IUCV_PEER,
		CONFIRM,
		ERROR,
		FINISH
	}
	state = SETUP;

	while (1)
	{
		enum state_wanted state_want = WANT_ERROR;

		switch(state)
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
#if 0
				switch (ret)
				{
					case WANT_NEXT:
						switch (type)
						{
							case TYPE_IUCV:
								state = GET_QETH_PORTNAME_IUCV_PEER;
								break;
							default:
								state = GET_CHANNEL;
								break;
						}
						break;
					case WANT_BACKUP:
						state = BACKUP;
						break;
					default:
						state = WANT_ERROR;
				}
#endif
				break;
			case GET_CHANNEL:
				state_want = get_channel ();
#if 0
				switch (ret)
				{
					case 0:
						switch (type)
						{
							case TYPE_QETH:
							case TYPE_LCS:
								state = GET_QETH_LCS_PORT;
								break;
							case TYPE_CTC:
								state = GET_CTC_PROTOCOL;
								break;
							default:
								return 1;
						}
						break;
					case 30:
						state = GET_NETWORKTYPE;
						break;
					default:
						return 1;
				}
#endif
				break;
			case GET_CTC_PROTOCOL:
				state_want = get_ctc_protocol ();
#if 0
				switch (ret)
				{
					case 0:
						state = CONFIRM;
						break;
					case 30:
						state = GET_CHANNEL;
						break;
					default:
						return 1;
				}
#endif
				break;
			case GET_QETH_LCS_PORT:
				state_want = get_qeth_lcs_port ();
#if 0
				switch (ret)
				{
					case 0:
						switch (type)
						{
							case TYPE_QETH:
								state = GET_QETH_PORTNAME_IUCV_PEER;
								break;
							default:
								state = CONFIRM;
								break;
						}
						break;
					case 30:
						state = GET_CHANNEL;
						break;
					default:
						return 1;
				}
#endif
				break;
			case GET_QETH_PORTNAME_IUCV_PEER:
				state_want = get_qeth_portname_iucv_peer ();
#if 0
				switch (ret)
				{
					case 0:
						state = CONFIRM;
						break;
					case 1:
						switch (type)
						{
							case TYPE_QETH:
								state = CONFIRM;
								break;
							default:
								break;
						}
						break;
					case 30:
						state = GET_QETH_LCS_PORT;
						break;
					default:
						return 1;
				}
#endif
				break;
			case CONFIRM:
				state_want = confirm ();
#if 0
				switch (ret)
				{
					case 0:
						return setup ();
					case 1:
						state = GET_CHANNEL;
						break;
					case 30:
						switch (type)
						{
							case TYPE_QETH:
							case TYPE_IUCV:
								state = GET_QETH_PORTNAME_IUCV_PEER;
								break;
							case TYPE_CTC:
								state = GET_CTC_PROTOCOL;
								break;
							case TYPE_LCS:
								state = GET_QETH_LCS_PORT;
								break;
							default:
								return 1;
						}
						break;
				}
#endif
				break;
			case ERROR:
				state_want = error ();
				break;
			case FINISH:
				return 0;
		}

		switch (state_want)
		{
			case WANT_NONE:
				state = ERROR;
				break;
			case WANT_NEXT:
				switch (state)
				{
					case SETUP:
						state = DETECT_CHANNELS;
						break;
					case DETECT_CHANNELS:
						state = DETECT_DEVICES;
						break;
					default:
						state = ERROR;
				}
				break;
			default:
				state = ERROR;
		}
	}

	return 0;
}

/* vim: noexpandtab sw=8
 */
