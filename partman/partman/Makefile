CFLAGS=-Wall -Os -D_GNU_SOURCE
LIBS=-lparted -ldl -ldebian-installer

all: parted_server parted_devices map_devfs

parted_server: parted_server.c
	$(CC) $(CFLAGS) $(LIBS) parted_server.c -o parted_server

parted_devices: parted_devices.c
	$(CC) $(CFLAGS) $(LIBS) parted_devices.c -o parted_devices

map_devfs: map_devfs.c
	$(CC) $(CFLAGS) $(LIBS) map_devfs.c -o map_devfs

clean:
	rm -f parted_server parted_devices map_devfs

.PHONY: clean
