include ../../globalmakeflags

SUBDIRS=db frontend

all install clean: $(addsuffix /Makefile,$(SUBDIRS))
	@for d in $(SUBDIRS); do $(MAKE) -C $$d $@; done
	
