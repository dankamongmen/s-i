#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cdebconf/debconfclient.h>
#include <debian-installer.h>

#include <libsysfs.h>

const char *const file_devices = "/proc/dasd/devices";

#define TEMPLATE_PREFIX	"s390-dasd/"

static struct debconfclient *client;

static struct dasd
{
	char device[SYSFS_NAME_LEN];
	enum { UNKNOWN, NEW, UNFORMATTED, FORMATTED, READY } state;
}
*dasds, *dasd_current;

static unsigned int dasds_items;

enum state_wanted { WANT_BACKUP, WANT_NEXT, WANT_QUIT, WANT_ERROR };

int my_debconf_input(char *priority, char *template, char **ptr)
{
	int ret;
	debconf_input (client, priority, template);
	ret = debconf_go (client);
	debconf_get (client, template);
	*ptr = client->value;
	return ret;
}

static bool update_state (void)
{
	char buf[256];
	FILE *f = fopen(file_devices, "r");
	unsigned int i;

	if (!f)
		return false;

	while (fgets (buf, sizeof (buf), f))
	{
		for (i = 0; i < dasds_items; i++)
			if (strncmp (buf, dasds[i].device, strlen (dasds[i].device)) == 0)
			{
				if (!strncmp (buf + 48, "active", 6))
				{
					if (!strncmp (buf + 55, "n/f", 3))
						dasds[i].state = UNFORMATTED;
					else
						dasds[i].state = FORMATTED;
				}
				else if (!strncmp (buf + 48, "ready", 5))
					dasds[i].state = READY;
				else
					dasds[i].state = UNKNOWN;
				break;
			}
	}

	return true;
}

static enum state_wanted get_channel (void)
{
	char buf[256], *ptr;
	unsigned int i;
	int ret;
	struct dlist *devices;
	struct sysfs_bus *bus;
	struct sysfs_device *device;

	dasds = di_new (struct dasd, 5);
	dasd_current = NULL;
	dasds_items = 0;

	bus = sysfs_open_bus ("ccw");

	if (!bus)
		return WANT_ERROR;

	devices = sysfs_get_bus_devices (bus);

	dlist_for_each_data (devices, device, struct sysfs_device)
	{
		struct sysfs_attribute *devtype_attr = sysfs_get_device_attr (device, "devtype");
		unsigned int devtype;
		if (!devtype_attr)
			continue;
		if (sscanf (devtype_attr->value, "%4x/%*2x", &devtype) != 1)
			continue;
		if(devtype == 0x3390 ||
		   devtype == 0x3380 ||
		   devtype == 0x9345 ||
		   devtype == 0x9336 ||
		   devtype == 0x3370)
		{
			strcpy (dasds[dasds_items].device, device->bus_id);
			dasds[dasds_items].state = UNKNOWN;
			dasds_items++;
			if ((dasds_items % 5) == 0)
				dasds = di_renew (struct dasd, dasds, dasds_items + 5);
		}
	}

	if (!update_state ())
	{
		sysfs_close_bus (bus);
		return WANT_ERROR;
	}

	if (dasds_items > 20)
	{
		while (1)
		{
			ret = my_debconf_input ("high", TEMPLATE_PREFIX "choose", &ptr);

			for (i = 0; i < dasds_items; i++)
				if (strncmp (ptr, dasds[i].device, strlen (dasds[i].device)) == 0)
				{
					dasd_current = &dasds[i];
					break;
				}

			if (!dasd_current)
			{
				ret = my_debconf_input ("high", TEMPLATE_PREFIX "choose_invalid", &ptr);
				if (ret == 10)
				{
					sysfs_close_bus (bus);
					return WANT_BACKUP;
				}
			}
			else
				break;
		}
	}
	else if (dasds_items > 0)
	{
		buf[0] = '\0';
		for (i = 0; i < dasds_items; i++)
		{
			switch (dasds[i].state)
			{
				case NEW:
					ptr = "(new)";
					break;
				case UNFORMATTED:
					ptr = "(unformatted)";
					break;
				case FORMATTED:
					ptr = "(formatted)";
					break;
				case READY:
					ptr = "(ready)";
					break;
				default:
					ptr = "(unknown)";
			}
			di_snprintfcat (buf, sizeof (buf), "%s %s, ", dasds[i].device, ptr);
		}

		debconf_subst (client, TEMPLATE_PREFIX "choose_select", "choices", buf);
		ret = my_debconf_input ("high", TEMPLATE_PREFIX "choose_select", &ptr);

		if (ret == 10)
		{
			sysfs_close_bus (bus);
			return WANT_BACKUP;
		}
		if (!strcmp (ptr, "Quit"))
		{
			sysfs_close_bus (bus);
			return WANT_QUIT;
		}

		for (i = 0; i < dasds_items; i++)
			if (strncmp (ptr, dasds[i].device, strlen (dasds[i].device)) == 0)
			{
				dasd_current = &dasds[i];
				break;
			}
	}
	sysfs_close_bus (bus);
	return WANT_NEXT;
}

struct hd_geometry {
	unsigned char heads;
	unsigned char sectors;
	unsigned short cylinders;
	unsigned long start;
};

#define HDIO_GETGEO 0x0301  

static di_io_handler format_handler;

static int format_handler (const char *buf, size_t len, void *user_data __attribute__ ((unused)))
{
	if (len == 1 && buf[0] == '#')
		debconf_progress_step (client, 1);
	else
		di_log (DI_LOG_LEVEL_OUTPUT, "%s", buf);
	return 0;
}

static enum state_wanted confirm (void)
{
#if 0
	char buf[256], *ptr;
	int ret;
	bool needs_format = false;

	if (dasd_current->state == NEW)
	{
		snprintf (buf, sizeof (buf), "echo add %04x >/proc/dasd/devices", dasd_current->device);
		ret = di_exec_shell_log (buf);
		if (ret)
			return WANT_ERROR;
		update_state ();
	}

	snprintf (buf, sizeof (buf), "%04x", dasd_current->device);

	switch (dasd_current->state)
	{
		case UNFORMATTED:
		case READY:
			needs_format = true;
			debconf_subst (client, TEMPLATE_PREFIX "format", "device", buf);
			debconf_set (client, TEMPLATE_PREFIX "format", "true");
			ret = my_debconf_input ("medium", TEMPLATE_PREFIX "format", &ptr);
			break;
		case FORMATTED:
			debconf_subst (client, TEMPLATE_PREFIX "format_unclean", "device", buf);
			debconf_set (client, TEMPLATE_PREFIX "format_unclean", "false");
			ret = my_debconf_input ("critical", TEMPLATE_PREFIX "format_unclean", &ptr);
			break;
		default:
			return WANT_ERROR;
	}

	if (ret == 10 || (strcmp (ptr, "true") && needs_format))
		return WANT_BACKUP;

	if (strcmp (ptr, "true") == 0)
	{
		char dev[128];
		int fd;
		struct hd_geometry drive_geo;

		snprintf (dev, sizeof (dev), "/dev/dasd/%04x/device", dasd_current->device);

		fd = open (dev, O_RDONLY);
		if (fd < 0)
			return WANT_ERROR;
		if (ioctl (fd, HDIO_GETGEO, &drive_geo) < 0)
			return WANT_ERROR;
		close (fd);

		debconf_subst (client, TEMPLATE_PREFIX "formatting", "device", buf);
		debconf_progress_start (client, 0, drive_geo.cylinders, TEMPLATE_PREFIX "formatting");

		snprintf (buf, sizeof (buf), "dasdfmt -l LX%04x -b 4096 -m 1 -f %s -y", dasd_current->device, dev);
		ret = di_exec_shell_full (buf, format_handler, NULL, NULL, NULL, NULL, NULL, NULL);

		debconf_progress_stop (client);

		if (ret)
			return WANT_ERROR;
	}

	debconf_get (client, "debian-installer/kernel/commandline");
	strncpy (buf, client->value, sizeof (buf));

	ptr = strstr (buf, "dasd=");
	if (ptr)
	{
		char buf1[256] = "", buf2[256] = "";
		while (*ptr++ != '=');
		while (1)
		{ 
			unsigned int a = 0;
			sscanf (ptr, "%x", &a);
			if (a == dasd_current->device)
				return WANT_NEXT;;
			while (*++ptr && *ptr != ',' && !isspace (*ptr));
			if (*ptr != ',')
				break;
		}
		strncpy (buf1, buf, ptr - buf);
		strncpy (buf2, ptr, sizeof (buf2));
		snprintf (buf, sizeof (buf), "%s,%04x%s", buf1, dasd_current->device, buf2);
	}
	else
		di_snprintfcat (buf, sizeof (buf), " dasd=%04x", dasd_current->device);
	debconf_set (client, "debian-installer/kernel/commandline", buf);

#endif
	return WANT_NEXT;
}

static void error (void)
{
	char *ptr;

	my_debconf_input ("high", TEMPLATE_PREFIX "error", &ptr);
}

int main ()
{
	di_system_init ("s390-dasd");

	client = debconfclient_new ();
	debconf_capb (client, "backup");

	enum
	{
		BACKUP, GET_CHANNEL,
		CONFIRM, ERROR, QUIT
	}
	state = GET_CHANNEL;

	while (1)
	{
		enum state_wanted state_want = WANT_ERROR;

		switch (state)
		{
			case BACKUP:
				return 10;
			case GET_CHANNEL:
				state_want = get_channel ();
				break;
			case CONFIRM:
				state_want = confirm ();
				break;
			case ERROR:
				error ();
				state_want = WANT_QUIT;
			case QUIT:
				return 0;
		}
		switch (state_want)
		{
			case WANT_NEXT:
				switch (state)
				{
					case GET_CHANNEL:
						state = CONFIRM;
						break;
					case CONFIRM:
						state = GET_CHANNEL;
						break;
					default:
						state = ERROR;
						break;
				}
				break;
			case WANT_BACKUP:
				switch (state)
				{
					case GET_CHANNEL:
						state = BACKUP;
						break;
					case CONFIRM:
						state = GET_CHANNEL;
						break;
					default:
						state = ERROR;
						break;
				}
				break;
			case WANT_QUIT:
				state = QUIT;
				break;
			case WANT_ERROR:
				state = ERROR;
		}
	}
}

/* vim: noexpandtab sw=8
*/
