all:
clean:
	rm -f *~ demo.templates demo *stamp

install: 
	install -m 755 srm-reader ${DESTDIR}/usr/bin/srm-reader
	install -m 644 lang-codes $(DESTDIR)/usr/share/srm-reader/lang-codes

.PHONY: demo
	ln -sf debian/templates demo.templates
	chmod a+x debian/postinst
	rm -f demo
	ln -s debian/postinst demo
	DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./demo
	rm -f demo.templates demo

