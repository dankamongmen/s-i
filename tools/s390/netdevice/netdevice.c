#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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

struct channel {
	unsigned int device;
	unsigned int chantype;
	unsigned int cutype;
	unsigned int cumodel;
	unsigned int device_first;
};

struct device {
	unsigned int device_read;
	unsigned int device_write;
	unsigned int device_data;
	unsigned int chantype;
};

#define TEMPLATE_PREFIX	"debian-installer/s390/netdevice/"

static char *my_debconf_input (char *priority, char *question)
{
	char template[256];

	snprintf (template, 256, TEMPLATE_PREFIX "%s", question);

	debconf_fset(client, template, "seen", "false");
	debconf_input(client, priority, template);
	debconf_go(client);
	debconf_get(client, template);
	return client->value;
}

int channel_sort (const void *_s1, const void *_s2)
{
	struct channel *s1 = (struct channel *) _s1;
	struct channel *s2 = (struct channel *) _s2;
	if (s1->device < s2->device)
		return -1;
	else if (s1->device > s2->device)
		return 1;
	return 0;
}

#if 0
struct d_dasd *find_dasd (struct d_dasd *p, int i, char *s)
{
	int j;
	for (j = 0; j < i; j++)
		if (!strncasecmp(p[j].device, s, 4))
			return &p[j];
	return NULL;
}
#endif

int main (int argc, char *argv[])
{
	FILE *f;
	struct channel *channels;
	struct device *devices;
	size_t channels_items = 0, devices_items = 0;
	int type = 0, type_ctc = 0, type_escon = 0, type_lcs = 0, type_qeth = 0;
	int items = 0, items_ctc = 0, items_escon = 0, items_lcs = 0, items_qeth = 0;
	int i, j, k;
	char line[255], *type_text = "", chandev_parm[255], chandev_module_parm[255], *ptr, *ptr2;

	client = debconfclient_new ();
	// client->command (client, "title", "Network Device Configuration", NULL);

	ptr = my_debconf_input ("high", "choose_networktype");

	if (!strncmp (ptr, "iucv", 4))
		type_text = "iucv";
	else if (!strncmp (ptr, "ctc", 3))
		type_text = "ctc";
	else if (!strncmp (ptr, "lcs", 3))
		type_text = "lcs";
	else if (!strncmp (ptr, "qeth", 4))
		type_text = "qeth";

	if (!strcmp (type_text, "iucv"))
	{
	}
	else
	{
		channels = malloc (5 * sizeof (struct channel));

		f = fopen ("/proc/chandev", "r");

		if (f)
		{
			while (fgets (line, sizeof (line), f) && strncmp (line, "chan_type", 9));
			// I know its a hack
			ptr = line + 23;
			while ((ptr2 = strsep (&ptr, ",")))
			{
				sscanf (ptr2, "ctc=%x", &type_ctc);
				sscanf (ptr2, "escon=%x", &type_escon);
				sscanf (ptr2, "lcs=%x", &type_lcs);
				sscanf (ptr2, "qeth=%x", &type_qeth);
			}

			while (fgets (line, sizeof (line), f) && strncmp (line, "channels detected", 17));

			fgets (line, sizeof (line), f);
			fgets (line, sizeof (line), f);
			fgets (line, sizeof (line), f);

			while (fgets (line, sizeof (line), f))
				if (sscanf (line, "0x%*x 0x%4x 0x%2x 0x%4x 0x%2x",
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
		}
		else
			perror ("fopen");

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
				{
					k++;
				}
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
			exit (1);

		if (!items)
		{
			if (type == type_ctc)
				my_debconf_input ("high", "ctc/no");
			else if (type == type_lcs)
				my_debconf_input ("high", "lcs/no");
			else if (type == type_qeth)
				my_debconf_input ("high", "qeth/no");
			exit (0);
		}

		ptr = malloc (items * 16); // "0x0000-0x0000, "
		ptr[0] = '\0';

		for (i = 0; i < devices_items; i++)
			if (devices[i].chantype & type)
			{
				if (type == type_qeth)
					di_snprintfcat (ptr, items_ctc * 16, "0x%04x-0x%04x, ", devices[i].device_read, devices[i].device_data);
				else
					di_snprintfcat (ptr, items_ctc * 16, "0x%04x-0x%04x, ", devices[i].device_read, devices[i].device_write);
			}

		if (type == type_ctc)
			ptr2 = TEMPLATE_PREFIX "ctc/choose";
		else if (type == type_lcs)
			ptr2 = TEMPLATE_PREFIX "lcs/choose";
		else if (type == type_qeth)
			ptr2 = TEMPLATE_PREFIX "qeth/choose";

		debconf_subst (client, ptr2, "choices", ptr);
		free (ptr);
		debconf_fset (client, ptr2, "seen", "false");
		debconf_input (client, "medium", ptr2);
		debconf_go (client);
		debconf_get (client, ptr2);

		if (!strcmp (client->value, "Other"))
			exit (1);
		else if (!strcmp (client->value, "Quit"))
			exit (0);

		sscanf (client->value, "0x%x", &j);
		fprintf (stderr, "got: %x\n", j);

		for (i = 0; devices[i].device_read != j; i++);

		if (type == type_ctc)
		{
			ptr = TEMPLATE_PREFIX "ctc/confirm";

			ptr2 = my_debconf_input ("medium", "ctc/protocol");
			j = 0;
			if (!strcmp (ptr2, "Linux (1)"))
				j = 1;
			else if (!strcmp (ptr2, "Linux TTY (2)"))
				j = 2;
			else if (!strcmp (ptr2, "OS/390 (3)"))
				j = 3;

			sprintf (line, "%x", devices[i].device_data);
			debconf_subst (client, ptr, "protocol", ptr2);

			snprintf (chandev_module_parm, sizeof (chandev_module_parm), "ctc-1,0x%x,0x%x,0,%d",
				  devices[i].device_read, devices[i].device_write, j);
		}
		else if (type == type_lcs)
		{
			ptr = TEMPLATE_PREFIX "lcs/confirm";

			ptr2 =  my_debconf_input ("high", "lcs/port");
			sscanf (ptr2, "port %d", &j);
			sprintf (line, "%d", j);
			debconf_subst (client,  ptr, "port", line);
		}
		else if (type == type_qeth)
		{
			ptr = TEMPLATE_PREFIX "qeth/confirm";

			sprintf (line, "0x%x", devices[i].device_data);
			debconf_subst (client,  ptr, "device_data", line);

			ptr2 =  my_debconf_input ("high", "qeth/port");
			sscanf (ptr2, "port %d", &j);
			sprintf (line, "%d", j);
			debconf_subst (client,  ptr, "port", line);

			snprintf (chandev_module_parm, sizeof (chandev_module_parm), "qeth-1,0x%x,0x%x,0x%x,0,%d",
				  devices[i].device_read, devices[i].device_write, devices[i].device_data, j);

			ptr2 =  my_debconf_input ("high", "qeth/portname");
			j = strlen (ptr2);
			if (j)
			{
				for (k = 0; k < j; k++)
					ptr2[k] = toupper (ptr2[k]);
				snprintf (chandev_parm, sizeof (chandev_parm), "add_parms,0x%x,0x%x,0x%x,portname:%s",
					  type_qeth, devices[i].device_read, devices[i].device_data, ptr2);
			}
			else
				ptr2 = "-";
			debconf_subst (client,  ptr, "portname", ptr2);
		}

		sprintf (line, "0x%x", devices[i].device_read);
		debconf_subst (client,  ptr, "device_read", line);
		sprintf (line, "0x%x", devices[i].device_write);
		debconf_subst (client, ptr, "device_write", line);

		debconf_set (client, ptr, "true");
		debconf_fset (client, ptr, "seen", "false");
		debconf_input (client, "medium", ptr);
		debconf_go (client);
		debconf_get (client, ptr);

		if (!strcmp (client->value, "false"))
			exit (1);

		fprintf (stderr, "chandev_parm: %s\n", chandev_parm);
		fprintf (stderr, "chandev_module_parm: %s\n", chandev_module_parm);

		if (mkdir ("/etc/modutils", 777) && errno != EEXIST)
			perror ("mkdir");

		if (strlen (chandev_parm))
		{
			f = fopen ("/etc/modutils/0chandev.chandev", "a");

			if (f)
			{
				fprintf (f, "%s\n", chandev_parm);
				fclose (f);
			}
			else
				exit (1);

			f = fopen ("/proc/chandev", "a");

			if (f)
			{
				fprintf (f, "%s\n", chandev_parm);
				fclose (f);
			}
			else
				exit (1);
		}

		snprintf (line, sizeof (line), "/etc/modutils/%s.chandev", type_text);

		f = fopen (line, "a");

		if (f)
		{
			fprintf (f, "%s\n", chandev_module_parm);
			fclose (f);
		}
		else
		{
			perror ("fopen");
			exit (1);
		}

		f = fopen ("/proc/chandev", "a");

		if (f)
		{
			fprintf (f, "%s\n", chandev_module_parm);
			fprintf (f, "reprobe\n");
			fclose (f);
		}
		else
		{
			perror ("fopen");
			exit (1);
		}

		snprintf (line, sizeof (line), "modprobe %s", type_text);

		di_execlog (line);
	}

	return 0;
}

