# $Id: Makefile,v 1.1 2001/02/22 05:59:06 zw Exp $

# This is wrong 8:P  (Hey, big nose? Or girlie hair?)
SOBJ = corba.so
OBJS = corba.opic
include ../modules.mak

PROJ = dcf
PROJ_SERV = $(PROJ)-text
PROJ_SERVSRC = $($(PROJ)-text:%=%.c)
ORBIT_IDL = /usr/bin/orbit-idl
ORBIT_IDLFLAGS = --skeleton-impl
ORBIT_IDLTARGETS = $(PROJ)-stubs.c $(PROJ)-skels.c $(PROJ)-common.c $(PROJ).h
ORBIT_CFLAGS = -I/usr/lib/glib/include -I/usr/include
ORBIT_LIBS = -L/usr/lib -lORBit -lIIOP -lORBitutil -lglib -lm
CFLAGS += $(MODCFLAGS) $(ORBIT_CFLAGS)
LDFLAGS = $(MODLDFLAGS) $(ORBIT_LIBS)

$(PROJ)-all: $(ORBIT_IDLTARGETS) $(PROJ_SERV)

$(PROJ)-text: $(PROJ)-text.o $(PROJ)-skels.o $(PROJ)-common.o
	$(CC) -o $(PROJ)-text $< $(LDFLAGS)

$(ORBIT_IDLTARGETS): $(PROJ).idl
	$(ORBIT_IDL) $(ORBIT_IDLFLAGS) $<

$(PROJ_SERVSRC): $(PROJ).h

$(PROJ)-clean:
	-rm *.[oa] $(PROJ)-text

$(PROJ)-distclean: $(PROJ)-clean
	-rm -f $(PROJ)-stubs.[oc] $(PROJ)-skels.[oc] $(PROJ)-common.[oc] $(PROJ).h

.PHONY: $(PROJ)-all $(PROJ)-clean $(PROJ)-distclean
