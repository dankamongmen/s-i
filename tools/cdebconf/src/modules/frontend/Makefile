include ../../../globalmakeflags

MODULES=$(FRONTEND_MODULES)
SOBJS=$(addsuffix .so,$(FRONTEND_MODULES))

all: $(addsuffix /Makefile,$(SUBDIRS))
	@for module in $(MODULES); do \
		$(MAKE) -C $$module; \
		cp $$module/$$module.so .; \
	done

install: $(addsuffix /Makefile,$(SUBDIRS))
	@for module in $(MODULES); do \
		$(MAKE) -C $$module $@; \
	done

clean: $(addsuffix /Makefile,$(SUBDIRS))
	@for module in $(MODULES); do \
		$(MAKE) -C $$module clean; \
	done
	-@rm -f $(SOBJS)

.PHONY: all
