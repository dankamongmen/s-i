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

const char *const file_chandev = "/proc/chandev";
//const char *const file_chandev = "chandev";

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

static enum { TYPE_QETH, TYPE_CTC, TYPE_LCS, TYPE_IUCV } type;
static size_t channels_items, devices_items;
static int items, items_ctc, items_escon, items_lcs, items_qeth;
static int chantype_qeth;
static int device_selected, device_ctc_protocol, device_qeth_lcs_port;
static char *device_qeth_portname_iucv_peer;
static char *type_text = "", *module = "", chandev_parm[256], chandev_module_parm[256];

#define TEMPLATE_PREFIX	"s390-netdevice/"

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

static int get_networktype (void)
{
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
}

static int get_channel (void)
{
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
}

static int get_ctc_protocol (void)
{
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
}

static int get_qeth_lcs_port (void)
{
	char *ptr;
	int ret = my_debconf_input ("critical", TEMPLATE_PREFIX "qeth_lcs/port", &ptr);
	if (ret)
		return ret;

	sscanf (ptr, "%d", &device_qeth_lcs_port);

	return 0;
}

static int get_qeth_portname_iucv_peer (void)
{
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
}

static int confirm (void)
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
}

static int setup (void)
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
}

int main (int argc __attribute__ ((unused)), char *argv[] __attribute__ ((unused)))
{
	int ret;
	di_system_init ("s390-netdevice");

	client = debconfclient_new ();
	debconf_capb (client, "backup");

	enum
	{
		BACKUP, GET_NETWORKTYPE, GET_CHANNEL,
		GET_CTC_PROTOCOL, GET_QETH_LCS_PORT, GET_QETH_PORTNAME_IUCV_PEER,
		CONFIRM
	}
	state = GET_NETWORKTYPE;

	while (1)
	{
		switch(state)
		{
			case BACKUP:
				return 10;
			case GET_NETWORKTYPE:
				ret = get_networktype ();
				switch (ret)
				{
					case 0:
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
					case 30:
						state = BACKUP;
						break;
					default:
						return 1;
				}
				break;
			case GET_CHANNEL:
				ret = get_channel ();
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
				break;
			case GET_CTC_PROTOCOL:
				ret = get_ctc_protocol ();
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
				break;
			case GET_QETH_LCS_PORT:
				ret = get_qeth_lcs_port ();
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
				break;
			case GET_QETH_PORTNAME_IUCV_PEER:
				ret = get_qeth_portname_iucv_peer ();
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
				break;
			case CONFIRM:
				ret = confirm ();
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
				break;
		}
	}

	return 0;
}

/* vim: noexpandtab sw=8
 */
