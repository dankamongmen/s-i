include ../globalmakeflags

MAJOR=0
MINOR=1
MICRO=0
LDFLAGS=-L. -ldebconf
LIB=libdebconf.so
LIBNAME=libdebconf.so.$(MAJOR).$(MINOR).$(MICRO)
SONAME=libdebconf.so.$(MAJOR).$(MINOR)
BIN=debconf

LIBOBJS=commands.o confmodule.o frontend.o priority.o strutl.o configuration.o\
	database.o question.o template.o
BINOBJS=debconf.o

all: $(BIN) $(LIB) $(SONAME)

$(BIN): $(BINOBJS) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(BINOBJS) $(LDFLAGS)

$(LIBNAME): $(LIBOBJS)
	$(CC) $(CFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $^ -ldl

$(SONAME) $(LIB): $(LIBNAME)
	-ln -sf $^ $@

clean:
	-rm -f $(LIBOBJS) $(BINOBJS) $(BIN) $(LIB) $(LIBNAME) $(SONAME) *~

.PHONY: clean lib
