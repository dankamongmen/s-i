MAJOR=0
MINOR=1
MICRO=0
LIB=libdebian-installer.so
LIBNAME=$(LIB).$(MAJOR).$(MINOR).$(MICRO)
SONAME=$(LIB).$(MAJOR).$(MINOR)
LIBS=$(LIB) $(SONAME) $(LIBNAME)


OBJS=di_execlog.o di_log.o di_check_dir.o di_snprintfcat.o
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
#	$(CC) -c $(CFLAGS) $(DEFS) -o debian-installer.o debian-installer.c
	@echo Creating $(LIBNAME)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^ $(DEFS) $(CFLAGS)
	size $@ 

$(SONAME) $(LIB): $(LIBNAME)
	@ln -sf $^ $@

install:
	install -d  ${libdir}
	install -d  ${incdir}
	install -m 755 $(LIBNAME) ${libdir}
	ln -s $(LIBNAME) ${libdir}/$(SONAME)
	ln -s $(LIBNAME) ${libdir}/$(LIB)
	install -m 755 $(PIC_LIB) ${libdir}
	install -m 644 debian-installer.h ${incdir}


clean:
	rm -f *.o $(PIC_LIB) $(LIBS) 
