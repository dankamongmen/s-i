MAJOR=1
MINOR=0
MICRO=0
LIB=libdebian-installer.so
LIBNAME_A=libdebian-installer.a
LIBNAME=$(LIB).$(MAJOR).$(MINOR).$(MICRO)
SONAME=$(LIB).$(MAJOR)
LIBS=$(LIB) $(SONAME) $(LIBNAME) $(LIBNAME_A)


OBJS=di_prebaseconfig_append.o di_execlog.o di_log.o di_check_dir.o di_snprintfcat.o di_stristr.o di_pkg_parse.o
PIC_LIB=libdebian-installer_pic.a


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

all: $(LIBS) $(PIC_LIB)

$(PIC_LIB): $(OBJS)
	ar cqv $(PIC_LIB) $(OBJS)

DEFS=$(addprefix -DL__, $(subst .o,__,$(OBJS)))

di%.o: debian-installer.c
	$(CC) -c $(CFLAGS) -DL__$(subst .o,,$@)__ -o $@ debian-installer.c

$(LIBNAME): debian-installer.c
	@echo Creating $(LIBNAME)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^ $(DEFS) $(CFLAGS)
	size $@ 

$(LIBNAME_A): debian-installer.c
	$(CC) -c -Wall -Os -o debian-installer.o $^ $(DEFS)
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
	install -m 755 $(PIC_LIB) ${libdir}
	install -m 644 debian-installer.h ${incdir}


clean:
	rm -f *.o $(PIC_LIB) $(LIBS) 
