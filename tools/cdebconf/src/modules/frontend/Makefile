MODULES=$(FRONTEND_MODULES)
SOBJS=$(addsuffix .so,$(FRONTEND_MODULES))

all:
	@for module in $(MODULES); do \
		$(MAKE) -C $$module; \
		cp $$module/$$module.so .; \
	done

install:
	@for module in $(MODULES); do \
		$(MAKE) -C $$module $@; \
	done

clean:
	@for module in $(MODULES); do \
		$(MAKE) -C $$module clean; \
	done
	-@rm -f $(SOBJS)

.PHONY: all
