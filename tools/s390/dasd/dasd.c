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

struct d_dasd {
	char device[5];
	char devtype[5];
	int state;
};

char *my_debconf_input(char *priority, char *template)
{
	debconf_fset (client, template, "seen", "false");
	debconf_input (client, priority, template);
	debconf_go (client);
	debconf_get (client, template);
	return client->value;
}

struct d_dasd *find_dasd (struct d_dasd *p, int i, char *s)
{
	int j;
	for (j = 0; j < i; j++)
		if (!strncasecmp(p[j].device, s, 4))
			return &p[j];
	return NULL;
}

void state (struct d_dasd *dasds, int i)
{
	FILE *f = fopen("/proc/dasd/devices", "r");
	char line[255];
	int j;

	if (f)
	{
		while (fgets (line, sizeof (line), f))
			for (j = 0; j < i; j++)
				if (!strncasecmp (line, dasds[j].device, 4))
				{
					if (!strncmp (line + 40, "active", 6))
					{
						if (!strncmp (line + 47, "n/f", 3))
							dasds[j].state = 2;
						else
							dasds[j].state = 3;
					}
					else
						dasds[j].state = 1;
				}
	}
	else
		perror ("fopen");
}

int main(int argc, char *argv[])
{
	FILE *f;
	struct d_dasd *dasds, *cur;
	int items = 0, ret, i;
	char line[255], *ptr;

	client = debconfclient_new ();

	dasds = malloc (5*sizeof (struct d_dasd));

	f = fopen("/proc/subchannels", "r");
	if (f)
	{
		char device[5];
		char devtype[5];
		char inuse[4];
		fgets (line, sizeof line, f);
		fgets (line, sizeof line, f);
		while (fgets (line, sizeof (line), f))
			if (sscanf (line, "%4s %*4s %4s/%*2s %*4s/%*2s %s ", device, devtype, inuse) == 3)
				if(!strncmp (devtype, "3390", 4) ||
						!strncmp (devtype, "3380", 4) ||
						!strncmp (devtype, "9345", 4) ||
						!strncmp (devtype, "9336", 4) ||
						!strncmp (devtype, "3370", 4))
				{
					strncpy (dasds[items].device, device, 5);
					strncpy (dasds[items].devtype, devtype, 5);
					if (!strncmp(inuse, "yes", 3))
						dasds[items].state = 0;
					else
						dasds[items].state = 1;
					items++;
					if ((items%5)==0)
						dasds = realloc (dasds,(items+5)*sizeof (struct d_dasd));
				}
		fclose (f);
	}
	else
		perror ("fopen");

	state (dasds, items);

	if (items > 10)
	{
		ptr = my_debconf_input ("high", "debian-installer/s390/dasd/choose");

		if (!(cur = find_dasd (dasds, items, ptr)))
		{
			debconf_input (client, "high", "debian-installer/s390/dasd/choose_invalid");
			return 3;
		}
	}
	else if (items > 1)
	{
		line[0] = '\0';
		for (i = 0; i < items; i++)
		{
			strcat (line, dasds[i].device);
			switch (dasds[i].state)
			{
				case 1:
					strcat (line, " (new)");
					break;
				case 2:
					strcat (line, " (unformated)");
					break;
				case 3:
					strcat (line, " (formated)");
					break;
				default:
					strcat (line, " (unknown)");
			}
			strcat (line, ", ");
		}

		debconf_subst (client, "debian-install/s390/dasd/choose_select", "choices", line);
		my_debconf_input ("high", "debian-install/s390/dasd/choose_select");

		if (!strcmp (client->value, "Quit"))
			exit (0);

		cur = find_dasd (dasds, items, client->value);
	}
	else if (items)
		cur = dasds;
	else
		goto error;

	if (!cur)
		goto error;

	if (cur->state == 1)
	{
		snprintf (line, sizeof (line), "echo add %s >/proc/dasd/devices", cur->device);
		ret = system (line);
		state (dasds, items);
	}
	if (cur->state == 2)
	{
		debconf_subst (client, "debian-install/s390/dasd/format", "device", cur->device);
		debconf_set (client, "debian-install/s390/dasd/format", "true");
		ptr = my_debconf_input ("high", "debian-install/s390/dasd/format");
	}
	else if (cur->state == 3)
	{
		debconf_subst (client, "debian-install/s390/dasd/format_unclean", "device", cur->device);
		debconf_set (client, "debian-install/s390/dasd/format_unclean", "false");
		ptr = my_debconf_input ("critical", "debian-install/s390/dasd/format_unclean");
	}
	else
		goto error;

	if (!strncmp (ptr, "true", 3))
	{
		snprintf (line, sizeof (line), "dasdfmt -l LX%s -b 4096 -n %s -y", cur->device, cur->device);
		di_execlog (line);
	}

	return 0;

error:
	my_debconf_input ("high", "debian-install/s390/dasd/error");
	return 1;
}

