DEB_HOST_ARCH_OS    := $(shell dpkg-architecture -qDEB_HOST_ARCH_OS 2>/dev/null)

# Take account of old dpkg-architecture output.
ifeq ($(DEB_HOST_ARCH_OS),)
  DEB_HOST_ARCH_OS    := $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
endif

subdirs = \
	src

ifeq ($(DEB_HOST_ARCH_OS),linux)
subdirs += \
	src-bootfloppy
endif

build: build-recursive

install:
	@$(MAKE) install -C src DESTDIR=$(CURDIR)/debian/rootskel/
ifeq ($(DEB_HOST_ARCH_OS),linux)
	@$(MAKE) install -C src-bootfloppy DESTDIR=$(CURDIR)/debian/rootskel-bootfloppy/
endif

clean: clean-recursive

build-recursive clean-recursive:
	@target=`echo $@ | sed s/-recursive//`; \
	list='$(subdirs)'; \
	for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $$target) || exit 1; \
	done

