all: debian/templates check-scripts check-data

LIST=languagelist

debian/templates: mktemplates debian/templates-in $(LIST) $(LIST).l10n
	./mktemplates $(LIST) $(LIST).l10n $@ $@-in

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
