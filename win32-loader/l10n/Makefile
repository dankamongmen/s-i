# Add only languages really supported by NSIS there
# For others, a PO file may be added and kept until NSIS is translated
# Languages not yet supported by NSIS: ast bn dz kk ml ta te vi
LINGUAS= ar be bg bs ca cs de el en eo es eu fi fr ga gl he hr hu is it ja ko lt nb nl pl pt pt_BR ro ru sk sl sq sv th tr uk zh_CN zh_TW

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
