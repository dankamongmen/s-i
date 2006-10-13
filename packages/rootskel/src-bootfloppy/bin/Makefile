dir = bin
files_exec = \
	cpio

CC=klcc
CFLAGS= 
LDFLAGS=-shared -s
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=cpio
LIBS=


build-local:: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJS) $(LIBS)

clean:
	-rm -f $(BIN) $(OBJS) *~

include ../../Makefile.inc
