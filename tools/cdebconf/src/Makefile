include ../globalmakeflags

MAJOR=0
MINOR=1
MICRO=0
LDFLAGS=-L. -ldebconf -Wl,-rpath,$(shell pwd)
LIB=libdebconf.so
LIBNAME=libdebconf.so.$(MAJOR).$(MINOR).$(MICRO)
SONAME=libdebconf.so.$(MAJOR).$(MINOR)
DEBCONF=debconf
TOOLS=loadtemplate
BIN=$(DEBCONF) $(TOOLS)

LIBOBJS=commands.opic confmodule.opic frontend.opic priority.opic strutl.opic \
	configuration.opic database.opic question.opic template.opic

all: $(BIN) $(LIB) $(SONAME)
	(cd modules && make all)

debconf: debconf.o $(LIB)
	$(CC) $(CFLAGS) -o $@ debconf.o $(LDFLAGS)

loadtemplate: loadtemplate.o $(LIB)
	$(CC) $(CFLAGS) -o $@ loadtemplate.o $(LDFLAGS)

$(LIBNAME): $(LIBOBJS)
	$(CC) $(CFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $^ -ldl

$(SONAME) $(LIB): $(LIBNAME)
	-ln -sf $^ $@

clean:
	-rm -f $(LIBOBJS) $(BINOBJS) $(BIN) $(LIB) $(LIBNAME) $(SONAME) *~
	(cd modules && make clean)

.PHONY: clean lib
