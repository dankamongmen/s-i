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

static struct debconfclient *client;

static struct channel
{
	unsigned int device;
	unsigned int chantype;
	unsigned int cutype;
	unsigned int cumodel;
	unsigned int device_first;
}
*channels;

static struct device
{
	unsigned int device_read;
	unsigned int device_write;
	unsigned int device_data;
	unsigned int chantype;
}
*devices;

static size_t channels_items, devices_items;
static int type, type_ctc, type_escon, type_lcs, type_qeth;
static int items, items_ctc, items_escon, items_lcs, items_qeth;
static int device_selected, device_ctc_protocol, device_qeth_lcs_port;
static char *device_qeth_portname, *device_qeth_portname_display;
static char *type_text = "", chandev_parm[256], chandev_module_parm[256];

#define TEMPLATE_PREFIX	"debian-installer/s390/netdevice/"

static int my_debconf_input (char *priority, char *template, char **p)
{
	int ret;

	debconf_fset(client, template, "seen", "false");
	debconf_input(client, priority, template);
	ret = debconf_go(client);
	debconf_get(client, template);
	*p = client->value;
	return ret;
}

static int channel_sort (const void *_s1, const void *_s2)
{
	struct channel *s1 = (struct channel *) _s1;
	struct channel *s2 = (struct channel *) _s2;
	if (s1->device < s2->device)
		return -1;
	else if (s1->device > s2->device)
		return 1;
	return 0;
}

static bool get_networktype (void)
{
	char *ptr;
	int ret = my_debconf_input ("high", TEMPLATE_PREFIX "choose_networktype", &ptr);

	if (ret)
		return false;

	if (!strncmp (ptr, "iucv", 4))
		type_text = "iucv";
	else if (!strncmp (ptr, "ctc", 3))
		type_text = "ctc";
	else if (!strncmp (ptr, "lcs", 3))
		type_text = "lcs";
	else if (!strncmp (ptr, "qeth", 4))
		type_text = "qeth";

	return true;
}

static bool get_channel (void)
{
	FILE *f;
	unsigned int i, k;
	int j;
	char buf[256], *ptr, *ptr2;

	channels = di_new (struct channel, 5);
	channels_items = 0;

	f = fopen ("/proc/chandev", "r");

	if (!f)
		return false;

	while (fgets (buf, sizeof (buf), f) && strncmp (buf, "chan_type", 9));
	// I know its a hack
	ptr = buf + 23;
	while ((ptr2 = strsep (&ptr, ",")))
	{
		sscanf (ptr2, "ctc=%x", &type_ctc);
		sscanf (ptr2, "escon=%x", &type_escon);
		sscanf (ptr2, "lcs=%x", &type_lcs);
		sscanf (ptr2, "qeth=%x", &type_qeth);
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
			if (channels[channels_items].chantype & type_escon)
				channels[channels_items].chantype |= type_ctc;

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

		if (channels[i].chantype & type_qeth)
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
				if (channels[i].chantype & type_ctc)
					items_ctc++;
				if (channels[i].chantype & type_escon)
					items_escon++;
				if (channels[i].chantype & type_lcs)
					items_lcs++;
			}
		}

		if ((devices_items % 5) == 0)
			devices = realloc (devices, (devices_items + 5) * sizeof (struct device));
	}

	chandev_parm[0] = '\0';
	chandev_module_parm[0] = '\0';

	if (!strcmp (type_text, "ctc"))
	{
		type = type_ctc;
		items = items_ctc;
	}
	else if (!strcmp (type_text, "lcs"))
	{
		type = type_lcs;
		items = items_lcs;
	}
	else if (!strcmp (type_text, "qeth"))
	{
		type = type_qeth;
		items = items_qeth;
	}
	else
		return false;

	if (!items)
	{
		if (type == type_ctc)
			my_debconf_input ("high", TEMPLATE_PREFIX "ctc/no", &ptr);
		else if (type == type_lcs)
			my_debconf_input ("high", TEMPLATE_PREFIX "lcs/no", &ptr);
		else if (type == type_qeth)
			my_debconf_input ("high", TEMPLATE_PREFIX "qeth/no", &ptr);
		return false;
	}

	ptr = malloc (items * 16); // "0x0000-0x0000, "
	ptr[0] = '\0';

	for (i = 0; i < devices_items; i++)
		if (devices[i].chantype & type)
		{
			if (type == type_qeth)
				di_snprintfcat (ptr, items * 16, "0x%04x-0x%04x, ", devices[i].device_read, devices[i].device_data);
			else
				di_snprintfcat (ptr, items * 16, "0x%04x-0x%04x, ", devices[i].device_read, devices[i].device_write);
		}

	if (type == type_ctc)
		ptr2 = TEMPLATE_PREFIX "ctc/choose";
	else if (type == type_lcs)
		ptr2 = TEMPLATE_PREFIX "lcs/choose";
	else if (type == type_qeth)
		ptr2 = TEMPLATE_PREFIX "qeth/choose";
	else
		return false;

	debconf_subst (client, ptr2, "choices", ptr);
	free (ptr);
	debconf_fset (client, ptr2, "seen", "false");
	debconf_input (client, "medium", ptr2);
	debconf_go (client);
	debconf_get (client, ptr2);

	sscanf (client->value, "0x%x", &i);

	for (device_selected = 0; devices[device_selected].device_read != i; device_selected++);

	return true;
}

static bool get_ctc_protocol (void)
{
	char *ptr;
	int ret = my_debconf_input ("medium", TEMPLATE_PREFIX "ctc/protocol", &ptr);
	if (ret)
		return false;

	device_ctc_protocol = 0;
	if (!strcmp (ptr, "Linux (1)"))
		device_ctc_protocol = 1;
	else if (!strcmp (ptr, "Linux TTY (2)"))
		device_ctc_protocol = 2;
	else if (!strcmp (ptr, "OS/390 (3)"))
		device_ctc_protocol = 3;

	return true;
}

static bool get_qeth_lcs_port (void)
{
	char *ptr;
	int ret = my_debconf_input ("high", TEMPLATE_PREFIX "qeth_lcs/port", &ptr);
	if (ret)
		return false;

	sscanf (ptr, "%d", &device_qeth_lcs_port);

	return true;
}

static bool get_qeth_portname (void)
{
	char *ptr;
	int ret, j, k;

	ret = my_debconf_input ("high", TEMPLATE_PREFIX "qeth/portname", &ptr);
	if (ret)
		return false;

	free (device_qeth_portname);

	j = strlen (ptr);
	if (j)
	{
		di_log (DI_LOG_LEVEL_WARNING, "length: %d", j);
		device_qeth_portname = strdup (ptr);
		for (k = 0; k < j; k++)
			device_qeth_portname[k] = toupper (device_qeth_portname[k]);
		device_qeth_portname_display = device_qeth_portname;
	}
	else
	{
		device_qeth_portname = NULL; 
		device_qeth_portname_display = "-";
	}

	return true;
}

static bool confirm (void)
{
	char *template, buf[10], *ptr;
	int ret;

	if (type == type_ctc)
		template = TEMPLATE_PREFIX "ctc/confirm";
	else if (type == type_qeth)
		template = TEMPLATE_PREFIX "qeth/confirm";
	else if (type == type_lcs)
		template = TEMPLATE_PREFIX "lcs/confirm";
	else
		return false;

	if (type == type_ctc || type == type_qeth || type == type_lcs)
	{
		snprintf (buf, sizeof (buf), "0x%x", devices[device_selected].device_read);
		debconf_subst (client,  template, "device_read", buf);
		snprintf (buf, sizeof (buf), "0x%x", devices[device_selected].device_write);
		debconf_subst (client,  template, "device_write", buf);
	}
	if (type == type_qeth || type == type_lcs)
	{
		snprintf (buf, sizeof (buf), "%d", device_qeth_lcs_port);
		debconf_subst (client, template, "port", buf);
	}
	if (type == type_qeth)
	{
		snprintf (buf, sizeof (buf), "0x%x", devices[device_selected].device_data);
		debconf_subst (client,  template, "device_data", buf);
		debconf_subst (client,  template, "portname", device_qeth_portname_display);
	}

	debconf_set (client, template, "true");
	ret = my_debconf_input ("medium", template, &ptr);

	if (ret || !strstr (ptr, "true"))
		return false;

	if (type == type_ctc)
		snprintf (chandev_module_parm, sizeof (chandev_module_parm), "ctc-1,0x%x,0x%x,0,%d",
			  devices[device_selected].device_read,
			  devices[device_selected].device_write,
			  device_ctc_protocol);
	else if (type == type_qeth)
	{
		snprintf (chandev_module_parm, sizeof (chandev_module_parm), "qeth-1,0x%x,0x%x,0x%x,0,%d",
			  devices[device_selected].device_read,
			  devices[device_selected].device_write,
			  devices[device_selected].device_data,
			  device_qeth_lcs_port);
		if (device_qeth_portname)
			snprintf (chandev_parm, sizeof (chandev_parm), "add_parms,0x%x,0x%x,0x%x,portname:%s",
				  type_qeth,
				  devices[device_selected].device_read,
				  devices[device_selected].device_data,
				  device_qeth_portname);
	}
#if 0
	else if (type == type_lcs)
#endif

	return true;
}

static bool setup (void)
{
	FILE *f, *chandev;
	char buf[256];

	if (mkdir ("/etc/modutils", 777) && errno != EEXIST)
		perror ("mkdir");

	chandev = fopen ("/proc/chandev", "a");

	if (!chandev)
		exit (30);

	if (strlen (chandev_parm))
	{
		f = fopen ("/etc/modutils/0chandev.chandev", "a");

		if (f)
		{
			fprintf (f, "%s\n", chandev_parm);
			fclose (f);
		}
		else
			exit (30);

		fprintf (chandev, "%s\n", chandev_parm);
	}

	snprintf (buf, sizeof (buf), "/etc/modutils/%s.chandev", type_text);

	f = fopen (buf, "a");

	if (f)
	{
		fprintf (f, "%s\n", chandev_module_parm);
		fclose (f);
	}
	else
	{
		perror ("fopen");
		exit (30);
	}

	fprintf (chandev, "%s\n", chandev_module_parm);
	fprintf (chandev, "noauto\n");
	fprintf (chandev, "reprobe\n");
	fclose (chandev);

	snprintf (buf, sizeof (buf), "modprobe %s", type_text);

	di_exec_shell_log (buf);

	return true;
}

int main (int argc, char *argv[])
{
	di_system_init ("s390-netdevice");

	client = debconfclient_new ();
	debconf_capb (client, "backup");

	enum
	{
		BACKUP, GET_NETWORKTYPE, GET_IUCV, GET_CHANNEL,
		GET_CTC_PROTOCOL, GET_QETH_LCS_PORT, GET_QETH_PORTNAME,
		CONFIRM, QUIT
	}
	state = GET_NETWORKTYPE;

	while (1)
	{
		switch(state)
		{
			case BACKUP:
				return 10;
			case GET_NETWORKTYPE:
				if (!get_networktype ())
					state = BACKUP;
				else if (!strcmp (type_text, "iucv"))
					state = GET_IUCV;
				else
					state = GET_CHANNEL;
				break;
			case GET_IUCV:
				return 30;
			case GET_CHANNEL:
				if (!get_channel ())
					state = GET_NETWORKTYPE;
				else if (type == type_ctc)
					state = GET_CTC_PROTOCOL;
				else if (type == type_qeth || type == type_lcs)
					state = GET_QETH_LCS_PORT;
				else
					state = BACKUP;
				break;
			case GET_CTC_PROTOCOL:
				if (!get_ctc_protocol ())
					state = GET_CHANNEL;
				else
					state = CONFIRM;
				break;
			case GET_QETH_LCS_PORT:
				if (!get_qeth_lcs_port ())
					state = GET_CHANNEL;
				else if (type == type_qeth)
					state = GET_QETH_PORTNAME;
				else
					state = CONFIRM;
				break;
			case GET_QETH_PORTNAME:
				if (!get_qeth_portname ())
					state = GET_QETH_LCS_PORT;
				else
					state = CONFIRM;
				break;
			case CONFIRM:
				if (!confirm ())
					state = GET_CHANNEL;
				else
				{
					setup ();
					state = QUIT;
				}
				break;
			case QUIT:
				return 0;
		}
	}

	return 0;
}

