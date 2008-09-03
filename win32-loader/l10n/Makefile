LINGUAS=be bg ca cs de en es eu fi fr gl he hu it ja ko lt nb nl pl pt pt_BR ru sk sq sv th tr zh_CN zh_TW

all: templates/all.nsh templates/dialog.nsh po/messages.pot

templates/dialog.nsh: templates/all.nsh
	for i in $(LINGUAS) ; do \
		echo "Push \$${`LANGUAGE=$$i ./win32-loader.sh LANG_ENGLISH`}" ; \
		echo "Push \"`LANGUAGE=$$i ./win32-loader.sh English`\"" ; \
	done > $@

templates/all.nsh: $(foreach lang, $(LINGUAS), templates/$(lang).nsh)
	for i in $^ ; do echo "; BEGIN $$i" ; cat $$i ; echo "; END $$i" ; done > $@

win32-loader: win32-loader.c
	gcc -DTEXTDOMAINDIR=\"$(CURDIR)/locale\" $^ -o $@

templates/en.nsh: win32-loader.sh win32-loader
	mkdir -p templates
	LANGUAGE=C ./win32-loader.sh > $@

templates/%.nsh: po/%.po win32-loader.sh win32-loader po/messages.pot
	mkdir -p templates locale/$*/LC_MESSAGES
	msgmerge -U po/$*.po po/messages.pot
	msgfmt -c --statistics -o locale/$*/LC_MESSAGES/win32-loader.mo $<
	LANGUAGE=$* ./win32-loader.sh > $@

po/messages.pot: win32-loader.sh win32-loader.c
	xgettext --from-code=utf-8 -ctranslate --keyword=_ $^ -o $@

clean:
	rm -rf *~ templates locale win32-loader
