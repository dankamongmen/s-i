SUBDIRS=db frontend

all install clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d $@; done
	
