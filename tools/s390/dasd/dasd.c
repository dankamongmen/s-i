#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cdebconf/debconfclient.h>

static struct debconfclient *client;

struct d_dasd {
	char device[5];
	char devtype[5];
	int state;
};

char *debconf_input(char *priority, char *template)
{
	client->command(client, "fset", template, "seen", "false", NULL);
	client->command(client, "input", priority, template, NULL);
	client->command(client, "go", NULL);
	client->command(client, "get", template, NULL);
	return client->value;
}

struct d_dasd *find_dasd (struct d_dasd *p, int i, char *s)
{
	int j;
	for (j = 0; j <= i; j++)
		if (!strncasecmp(p[j].device, s, 4))
			return &p[j];
	return NULL;
}

void state (struct d_dasd *dasds, int i)
{
	FILE *f = fopen("/proc/dasd/devices", "r");
	char line[255];
	int j;

	if (f) {
		while (fgets (line, sizeof (line), f))
			for (j = 0; j < i; j++)
				if (!strncasecmp(line, dasds[j].device, 4)) {
					if (!strncmp(line + 40, "active", 6)) {
						if (!strncmp(line + 47, "n/f", 3))
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

	client = debconfclient_new();
	client->command(client, "title", "DASD Configuration", NULL);

	dasds = malloc(5*sizeof(struct d_dasd));

	f = fopen("/proc/subchannels", "r");
	if (f) {
		char device[5];
		char devtype[5];
		char inuse[4];
		fgets(line, sizeof line, f);
		fgets(line, sizeof line, f);
		while (fgets (line, sizeof (line), f))
			if (sscanf(line, "%4s %*4s %4s/%*2s %*4s/%*2s %s ", device, devtype, inuse) == 3)
				if(!strncmp (devtype, "3390", 4) || !strncmp (devtype, "3380", 4) ||
						!strncmp (devtype, "9345", 4) || !strncmp (devtype, "9336", 4) ||
						!strncmp (devtype, "3370", 4)) {
					strncpy (dasds[items].device, device, 5);
					strncpy (dasds[items].devtype, devtype, 5);
					if (!strncmp(inuse, "yes", 3))
						dasds[items].state = 0;
					else
						dasds[items].state = 1;
					items++;
					if ((items%5)==0) {
						dasds = realloc(dasds,(items+5)*sizeof(struct d_dasd));
					}
				}
		fclose(f);
	}
	else
		perror ("fopen");

	state (dasds, items);

	if (items > 10) {
		ptr = debconf_input ("critical", "s390/dasd/choose");

		if (!(cur = find_dasd (dasds, items, ptr)))
			client->command(client, "input", "critical", "s390/dasd/choose_invalid", NULL);
	}
	else
	{
		line[0] = '\0';
		for (i = 0; i < items; i++) {
			strcat (line, dasds[i].device);
			switch (dasds[i].state) {
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

		client->command(client, "subst", "s390/dasd/choose_select", "choices", line, NULL);
		client->command(client, "input", "critical", "s390/dasd/choose_select", NULL);
		client->command(client, "go", NULL);
		client->command(client, "get", "s390/dasd/choose_select", NULL);

		cur = find_dasd (dasds, items, client->value);
	}

	if (cur->state == 0) {
		snprintf (line, sizeof (line), "echo add %s >/proc/dasd/devices", cur->device);
		ret = system (line);
		state (dasds, items);
	}
	if (cur->state == 2) {
		client->command(client, "subst", "s390/dasd/format", "device", cur->device, NULL);
		ptr = debconf_input ("critical", "s390/dasd/format");
	}
	else if (cur->state == 3) {
		client->command(client, "subst", "s390/dasd/format_unclean", "device", cur->device, NULL);
		ptr = debconf_input ("critical", "s390/dasd/format_unclean");
	}
	else
		return 1;

	if (!strncmp (ptr, "true", 3)) {
		snprintf (line, sizeof(line), "echo yes | dasdfmt -l LX%s -b 4096 -n %s >/tmp/dasdfmt.log 2>& 2>&11", cur->device, cur->device);
		ret = system (line);
	}

	return 0;
}
