all:

.PHONY: demo
demo:
	ln -sf debian/templates demo.templates
	chmod a+x debian/postinst
	rm -f demo
	ln -s debian/postinst demo
	DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./demo
	rm -f demo.templates demo

clean:
