include ../../../globalmakeflags

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

XGETTEXT = xgettext
TEMPLATES = \
   $(srcdir)/debian/cdebconf-gtk-udeb.templates \
   $(srcdir)/debian/cdebconf-slang-udeb.templates \
   $(srcdir)/debian/cdebconf-text-udeb.templates

templates: $(TEMPLATES)

$(srcdir)/debian/cdebconf-gtk-udeb.templates: gtk/gtk.c
$(srcdir)/debian/cdebconf-slang-udeb.templates: slang/slang.c
$(srcdir)/debian/cdebconf-text-udeb.templates: text/text.c

$(TEMPLATES):
	$(XGETTEXT) -c --keyword=get_text:2,3 --omit-header --no-location $? -o - | $(srcdir)/debian/po2templates > $@

.PHONY: all templates
