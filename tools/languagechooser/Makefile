all:

debian/templates: Makefile debian/templates.in languagelist
	languagenames=`grep -v "^#" languagelist | cut -d\; -f1 | tr "\n" "," | sed 's/,/, /g' | sed "s/, $$//g"`; \
	sed "s/@languagenames@/$$languagenames/" < debian/templates.in > $@


.PHONY: demo
languagechooser.templates: debian/templates
	(cd debian; po2debconf templates) > $@
demo: languagechooser.templates languagechooser
	DEBIAN_FRONTEND=dialog DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./languagechooser

clean:
