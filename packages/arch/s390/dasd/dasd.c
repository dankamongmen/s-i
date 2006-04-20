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

enum dasd_type { DASD_TYPE_ECKD, DASD_TYPE_FBA };

struct dasd
{
	char name[SYSFS_NAME_LEN];
	char devtype[SYSFS_NAME_LEN];
	enum dasd_type type;
};

static di_tree *dasds;

static struct dasd *dasd_current;

enum state_wanted { WANT_NONE = 0, WANT_BACKUP, WANT_NEXT, WANT_FINISH, WANT_ERROR };

int my_debconf_input(char *priority, char *template, char **ptr)
{
	int ret;
	debconf_input (client, priority, template);
	ret = debconf_go (client);
	debconf_get (client, template);
	*ptr = client->value;
	return ret;
}

static di_compare_func dasd_compare;

int dasd_device (const char *i)
{
	unsigned int ret;
	if (sscanf (i, "0.0.%04x", &ret) == 1)
		return ret;
	if (sscanf (i, "%04x", &ret) == 1)
		return ret;
	return -1;
}

int dasd_compare (const void *key1, const void *key2)
{
	return dasd_device ((const char *) key1) - dasd_device ((const char *) key2);
}

#if 0
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
#endif

static enum state_wanted detect_channels_driver (struct sysfs_driver *driver, enum dasd_type type)
{
	struct dlist *devices;
	struct sysfs_device *device;

	devices = sysfs_get_driver_devices (driver);

	dlist_for_each_data (devices, device, struct sysfs_device)
	{
		struct sysfs_attribute *devtype_attr;
		struct dasd *current;

		devtype_attr = sysfs_get_device_attr (device, "devtype");
		if (!devtype_attr)
			return WANT_NONE;
		current = di_new (struct dasd, 1);
		if (!current)
			return WANT_ERROR;
		strncpy (current->name, device->name, sizeof (current->name));
		sysfs_read_attribute (devtype_attr);
		strncpy (current->devtype, devtype_attr->value, sizeof (current->devtype));
		current->type = type;
		di_tree_insert (dasds, current, current);
	}

	return WANT_NONE;
}

static enum state_wanted detect_channels (void)
{
	struct sysfs_driver *driver;
	enum state_wanted ret;
	unsigned int i;
	const struct {
		const char *name;
		enum dasd_type type;
	} drivers[] = {
		{ "dasd-eckd", DASD_TYPE_ECKD },
		{ "dasd-fba", DASD_TYPE_FBA },
	};

	dasds = di_tree_new (dasd_compare);

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

static enum state_wanted get_channel_input (void)
{
	int ret;
	char *ptr;

	while (1)
	{
		ret = my_debconf_input ("high", TEMPLATE_PREFIX "choose", &ptr);
		if (ret == 10)
			return WANT_BACKUP;

		dasd_current = di_tree_lookup (dasds, ptr);
		if (dasd_current)
			return WANT_NEXT;

		ret = my_debconf_input ("high", TEMPLATE_PREFIX "choose_invalid", &ptr);
		if (ret == 10)
			return WANT_BACKUP;
	}
}

static di_hfunc get_channel_select_append;
static void get_channel_select_append (void *key, void *value __attribute__ ((unused)), void *user_data)
{
	const char *name = key;
	char *buf = user_data;
	di_snprintfcat (buf, 512, "%s, ", name);
}

static enum state_wanted get_channel_select (void)
{
	char buf[512], *ptr;
	int ret;

	buf[0] = '\0';
	di_tree_foreach (dasds, get_channel_select_append, buf);

	debconf_subst (client, TEMPLATE_PREFIX "choose_select", "choices", buf);
	ret = my_debconf_input ("high", TEMPLATE_PREFIX "choose_select", &ptr);

	if (ret == 10)
		return WANT_BACKUP;
	if (!strcmp (ptr, "Finish"))
		return WANT_FINISH;

	dasd_current = di_tree_lookup (dasds, ptr);
	if (dasd_current)
		return WANT_NEXT;
	return WANT_ERROR;
}

static enum state_wanted get_channel (void)
{
	if (di_tree_size (dasds) > 20)
		return get_channel_input ();
	else if (di_tree_size (dasds) > 0)
		return get_channel_select ();
	return WANT_ERROR;
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
		BACKUP, DETECT_CHANNEL, GET_CHANNEL,
		CONFIRM, ERROR, FINISH
	}
	state = DETECT_CHANNEL;

	while (1)
	{
		enum state_wanted state_want = WANT_ERROR;

		switch (state)
		{
			case BACKUP:
				return 10;
			case DETECT_CHANNEL:
				state_want = detect_channels ();
				break;
			case GET_CHANNEL:
				state_want = get_channel ();
				break;
			case CONFIRM:
				state_want = confirm ();
				break;
			case ERROR:
				error ();
				state_want = WANT_FINISH;
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
					case DETECT_CHANNEL:
						state = GET_CHANNEL;
						break;
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
			case WANT_FINISH:
				state = FINISH;
				break;
			case WANT_ERROR:
				state = ERROR;
		}
	}
}

/* vim: noexpandtab sw=8
*/
