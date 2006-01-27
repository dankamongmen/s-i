MODULE=newt
SOBJ=newt.so
OBJS=newt.opic

MODLDFLAGS=-lnewt

include ../modules.mak

install::
ifneq ($(TARGET),udeb)
	install -m 644 ${srcdir}/src/modules/frontend/newt/config-newt.h $(DESTDIR)${incdir}
endif
