MAJOR=0
MINOR=1
MICRO=0
LIB=libd-i.so
LIBNAME=$(LIB).$(MAJOR).$(MINOR).$(MICRO)
SONAME=$(LIB).$(MAJOR).$(MINOR)
LIBS=$(LIB) $(SONAME) $(LIBNAME)


OBJS=di_execlog.o di_log.o di_check_dir.o
PIC_LIB=libd-i_pic.a


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

di%.o: d-i.c
	$(CC) -c $(CFLAGS) -DL__$(subst .o,,$@)__ -o $@ d-i.c

$(LIBNAME): d-i.c
#	$(CC) -c $(CFLAGS) $(DEFS) -o d-i.o d-i.c
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
	install -m 644 d-i.h ${incdir}
	


clean:
	rm -f *.o $(PIC_LIB) $(LIBS) 
