LINGUAS=bg ca cs de en es fi fr he hu it ja nl pt ru sk sv tr

all: $(foreach lang, $(LINGUAS), templates/$(lang).nsh) po/messages.pot

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
