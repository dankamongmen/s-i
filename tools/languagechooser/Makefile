all: debian/templates check-scripts

debian/templates: Makefile debian/templates.in languagelist
	languagenames=`grep -v "^#" languagelist | cut -d\; -f1 | tr "\n" "," | sed 's/,/, /g' | sed "s/, $$//g"`; \
	sed "s/@languagenames@/$$languagenames/" < debian/templates.in > $@

check-scripts:
	if [ -x /bin/ash ] ; then SH=ash ; else SH=dash; fi ; \
	for s in debian/postinst prebaseconfig \
	   languagemap languagechooser ; do \
		$$SH -n $$s ; \
	done

.PHONY: demo
languagechooser.templates: debian/templates
	(cd debian; po2debconf templates) > $@
demo: languagechooser.templates languagechooser
	DEBIAN_FRONTEND=dialog DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./languagechooser

clean:
	$(RM) languagechooser.templates debian/templates
