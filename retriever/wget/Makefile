ARCH=$(shell dpkg --print-architecture)
CFLAGS=-Wall -g -D_GNU_SOURCE -DARCH=\"$(ARCH)\"
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=wget-retriever
LIBS=-ldebconf
# We're sticking at sid for now, since the udebs are not in woody.
# This must be kept up-to-date to whatever the name of the release of
# debian is. TODO: paramterize somehow, outside of this retreiver?
DISTNAME=sid

ifdef DEBUG
CFLAGS:=$(CFLAGS) -DDODEBUG
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

# Size optimized and stripped binary target.
small: CFLAGS=-Os $(CFLAGS) -DSMALL
small: clean $(BIN)
	strip --remove-section=.comment --remove-section=.note $(BIN)
	ls -l $(BIN)

clean:
	-rm -f $(BIN) $(OBJS) *~
