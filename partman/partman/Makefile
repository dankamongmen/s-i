CFLAGS=-Wall -Os -D_GNU_SOURCE
LIBS=-lparted -ldl

all: parted_server parted_devices

parted_server: parted_server.c
	$(CC) $(CFLAGS) $(LIBS) parted_server.c -o parted_server

parted_devices: parted_devices.c
	$(CC) $(CFLAGS) $(LIBS) parted_devices.c -o parted_devices

clean:
	rm -f parted_server parted_devices

.PHONY: clean
