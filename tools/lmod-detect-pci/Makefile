PROG = lmod-detect-pci 
CC = gcc
CFLAGS = -Wall -Os -fomit-frame-pointer
INSTALL = install
LIBS = -lpci
DESTDIR = 
STRIPTOOL = strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment $(PROG)
SOURCE = lmod-detect-pci.c 
OBJ = lmod-detect-pci.o

all: $(PROG) 

$(PROG): $(OBJ) 
	$(CC) -o $(PROG) $(OBJ) $(LIBS) 
	$(STRIP)

$(OBJ): $(SOURCE) pci_ids.h 
	$(CC) $(CFLAGS) -c $(SOURCE)

clean:
	-rm -f $(PROG) $(OBJ)

install: $(PROG) 
	$(INSTALL) -d -m 755 $(DESTDIR)/usr/bin
	$(INSTALL) -s -m 755 $(PROG) $(DESTDIR)/usr/bin/$(PROG) 

