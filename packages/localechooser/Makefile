all: debian/SUPPORTED-short debian/short-tmp/shortlists debian/templates check-scripts check-data check-utf8

LIST     = languagelist
L10NLIST = languagelist.l10n

debian/templates.base: mktemplates.language debian/localechooser.templates-in $(LIST) $(L10NLIST)
	LC_COLLATE=C ./mktemplates.language $(LIST) $(L10NLIST) debian/templates.lang debian/localechooser.templates-in
	./mktemplates.country debian/templates debian/templates.lang

debian/iso-codes: 
	./get-iso-codes

debian/templates: debian/templates.base debian/short-tmp/shortlists
	./mk_shortlist_templates

debian/SUPPORTED-short:
	./get-SUPPORTED

debian/short-tmp/shortlists: debian/iso-codes
	./mkshort

check-scripts:
	if [ -x /bin/ash ] ; then SH=ash ; else SH=dash; fi ; \
	for s in prebaseconfig debian/postinst languagemap localechooser ; do \
		$$SH -n $$s ; \
	done

check-utf8: $(L10NLIST)
	iconv -f utf-8 -t unicode $(L10NLIST) > /dev/null

check-data:
	# Check that the listed locale is supported, to make sure it will work.
	@for locale in `grep -v "^#" $(LIST) | cut -d\; -f2 | grep _` ; do \
		if grep -q "^$$locale " /usr/share/i18n/SUPPORTED ; then \
			: ; \
		else \
			echo "warning: locale $$locale not listed in /usr/share/i18n/SUPPORTED" ; \
		fi ; \
	done

.PHONY: demo
localechooser.templates: debian/templates
	grep -v '#' $^ > $@
demo: localechooser.templates localechooser
	DEBIAN_FRONTEND=dialog DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./localechooser

clean:
	$(RM) localechooser.templates debian/templates debian/templates.lang debian/templates.tmp debian/templates.base SUPPORTED-short
	$(RM) -rf debian/short-tmp debian/iso-codes
