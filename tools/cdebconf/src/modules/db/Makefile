include ../../../globalmakeflags

SOBJS=textdb.so
OBJS=textdb.o

CFLAGS += -I../..

all: $(SOBJS)

textdb.so: textdb.o
	$(CC) $(CFLAGS) -shared -o $@ $^

clean:
	-rm -f $(SOBJS) $(OBJS) *~
