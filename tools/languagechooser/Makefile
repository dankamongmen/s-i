all:

.PHONY: demo
demo:
	(cd debian; po2debconf templates) > languagechooser.templates
	DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./languagechooser

clean:
