ifndef TARGETS
TARGETS=netcfg-dhcp netcfg-static
endif

MAJOR=0
MINOR=1
MICRO=0
LIB=libd-i.so
LIBNAME=$(LIB).$(MAJOR).$(MINOR).$(MICRO)
SONAME=$(LIB).$(MAJOR).$(MINOR)

LIBS=$(LIB) $(SONAME) $(LIBNAME)

PREFIX=$(DESTDIR)/usr/
libdir=$(DESTDIR)/usr/lib/
incdir=$(DESTDIR)/usr/include/
CFLAGS=-Wall  -Os -fomit-frame-pointer
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment

all: $(LIBS) 


$(LIBNAME): d-i.o
	@echo Creating $(LIBNAME)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^
	size $@ 

$(SONAME) $(LIB): $(LIBNAME)
	@ln -sf $^ $@

install:
	install -d  ${libdir}
	install -d  ${incdir}
	install -m 755 $(LIBNAME) ${libdir}
	ln -s $(LIBNAME) ${libdir}/$(SONAME)
	ln -s $(LIBNAME) ${libdir}/$(LIB)
	install -m 644 d-i.h ${incdir}

clean:
	rm -f *.o $(LIBS) 
