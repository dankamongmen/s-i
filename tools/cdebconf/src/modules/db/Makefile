MODULES=textdb perldb
SOBJS=$(addsuffix .so,$(MODULES))

all: $(SOBJS)

%.so:
	@$(MAKE) -C $*
	@cp $*/$*.so .

clean:
	@for module in $(MODULES); do \
		$(MAKE) -C $$module clean; \
	done
	-@rm -f $(SOBJS)
