all: debian/templates check-scripts check-data

LIST=languagelist

debian/templates: Makefile debian/templates-in $(LIST)
	languagenames=`grep -v "^#" $(LIST) | cut -d\; -f1 | tr "\n" "," | sed "s/,$$//g"`; \
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

check-data:
	# Check that the listed locale is supported, to make sure base-config know
	# how to handle it.
	@for locale in `grep -v "^#" languagelist | cut -d\; -f2` ; do \
		if grep -q "^$$locale " /usr/share/i18n/SUPPORTED ; then \
			: ; \
		else \
			echo "warning: locale $$locale not listed in /usr/share/i18n/SUPPORTED" ; \
		fi ; \
	done

.PHONY: demo
languagechooser.templates: debian/templates
	cat $^ > $@
demo: languagechooser.templates languagechooser
	DEBIAN_FRONTEND=dialog DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./languagechooser

clean:
	$(RM) languagechooser.templates debian/templates
