all: debian/templates check-scripts

debian/templates: Makefile debian/templates-in languagelist
	languagenames=`grep -v "^#" languagelist | cut -d\; -f1 | tr "\n" "," | sed "s/,$$//g"`; \
	save_IFS=$$IFS; IFS=','; \
	for l in $$languagenames; do \
	  msg=`grep "^$${l}: " languagelist.l10n | sed -e "s/^$${l}: //"`;\
	  if [ -n "$$msg" ]; then nativemessages=$$nativemessages", "$$msg; else { echo Missing $$l in languagelist.l10n; exit 2;} fi; \
	done; \
	IFS=$$save_IFS; \
	languagenames=`echo $$languagenames | sed -e 's/^,//' -e 's/,/, /g'`; \
	nativemessages=`echo $$nativemessages | sed -e 's/^, //'`; \
	sed -e "s/@languagenames@/$$languagenames/" -e "s/@nativemessages@/$$nativemessages/" < debian/templates-in > $@

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
