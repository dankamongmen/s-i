MAJOR=3
MINOR=4
MICRO=0
LIB=libdebian-installer.so
LIBNAME_A=libdebian-installer.a
LIBNAME=$(LIB).$(MAJOR).$(MINOR).$(MICRO)
SONAME=$(LIB).$(MAJOR)
LIBS=$(LIB) $(SONAME) $(LIBNAME) $(LIBNAME_A)


PREFIX=$(DESTDIR)/usr/
libdir=$(DESTDIR)/usr/lib/
incdir=$(DESTDIR)/usr/include/
CFLAGS=-fPIC -Wall  -Os 

ifneq (,$(findstring debug,$(DEB_BUILD_OPTIONS)))
CFLAGS += -g
else
CFLAGS += -fomit-frame-pointer
endif


INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment

all: $(LIBS)

$(LIBNAME): debian-installer.c
	@echo Creating $(LIBNAME)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^ $(CFLAGS)
	size $@ 

$(LIBNAME_A): debian-installer.c
	$(CC) -c -Wall -Os -o debian-installer.o $^
	$(AR) cr $@ debian-installer.o

$(SONAME) $(LIB): $(LIBNAME)
	@ln -sf $^ $@

install:
	install -d  ${libdir}
	install -d  ${incdir}
	install -m 755 $(LIBNAME) ${libdir}
	install -m 755 $(LIBNAME_A) ${libdir}
	ln -sf $(LIBNAME) ${libdir}/$(SONAME)
	ln -sf $(LIBNAME) ${libdir}/$(LIB)
	install -m 644 debian-installer.h ${incdir}


clean:
	rm -f *.o $(LIB) $(LIBNAME_A) $(LIB).*
