MODULES=perldb textdb
SOBJS=$(addsuffix .so,$(MODULES))

all:
	@for module in $(MODULES); do \
		$(MAKE) -C $$module; \
		cp $$module/$$module.so .; \
	done

clean:
	@for module in $(MODULES); do \
		$(MAKE) -C $$module clean; \
	done
	-@rm -f $(SOBJS)

.PHONY: all
