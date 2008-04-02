subdirs = \
	src \
	src-bootfloppy

build: build-recursive

install:
	@$(MAKE) install -C src DESTDIR=$(CURDIR)/debian/rootskel/
	@$(MAKE) install -C src-bootfloppy DESTDIR=$(CURDIR)/debian/rootskel-bootfloppy/

clean: clean-recursive

build-recursive clean-recursive:
	@target=`echo $@ | sed s/-recursive//`; \
	list='$(subdirs)'; \
	for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $$target) || exit 1; \
	done

