include ../../../globalmakeflags

SOBJS=textdb.so
OBJS=textdb.opic

CFLAGS += -I../..

all: $(SOBJS)

textdb.so: textdb.opic
	$(CC) $(CFLAGS) -shared -o $@ $^

clean:
	-rm -f $(SOBJS) $(OBJS) *~
