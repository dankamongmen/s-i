all: debian/templates check-scripts check-data check-utf8

LIST     = languagelist
L10NLIST = languagelist.l10n

debian/templates: mktemplates debian/templates-in $(LIST) $(L10NLIST)
	LC_COLLATE=C ./mktemplates $(LIST) $(L10NLIST) $@ $@-in

check-scripts:
	if [ -x /bin/ash ] ; then SH=ash ; else SH=dash; fi ; \
	for s in prebaseconfig debian/postinst languagemap languagechooser ; do \
		$$SH -n $$s ; \
	done

check-utf8: $(L10NLIST)
	iconv -f utf-8 -t unicode $(L10NLIST) > /dev/null

check-data:
	# Check that the listed locale is supported, to make sure base-config knows
	# how to handle it.
	@for locale in `grep -v "^#" $(LIST) | cut -d\; -f2 | grep _` ; do \
		if grep -q "^$$locale " /usr/share/i18n/SUPPORTED ; then \
			: ; \
		else \
			echo "warning: locale $$locale not listed in /usr/share/i18n/SUPPORTED" ; \
		fi ; \
	done

.PHONY: demo
languagechooser.templates: debian/templates
	grep -v '#' $^ > $@
demo: languagechooser.templates languagechooser
	DEBIAN_FRONTEND=dialog DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./languagechooser

clean:
	$(RM) languagechooser.templates debian/templates
