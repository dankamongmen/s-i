SOBJ=perldb.so
OBJS=perldb.opic

MODCFLAGS:= $(shell perl -MExtUtils::Embed -e ccopts)
MODLDFLAGS:= $(shell perl -MExtUtils::Embed -e ldopts)

include ../modules.mak

# Only install in full version
ifdef ($(TARGET),udeb)
INSTALLOBJ=
else
INSTALLOBJ=$(SOBJ)
endif

