subdirs = \
	src \
	src-bootfloppy \
	src-locale

build: build-recursive

install:
	@$(MAKE) install -C src DESTDIR=$(CURDIR)/debian/rootskel/
	@$(MAKE) install -C src-bootfloppy DESTDIR=$(CURDIR)/debian/rootskel-bootfloppy/
	@$(MAKE) install -C src-locale DESTDIR=$(CURDIR)/debian/rootskel-locale/

clean: clean-recursive

build-recursive clean-recursive:
	@target=`echo $@ | sed s/-recursive//`; \
	list='$(subdirs)'; \
	for subdir in $$list; do \
	  echo "Making install in $$subdir"; \
	  (cd $$subdir && $(MAKE) $$target) || exit 1; \
	done

